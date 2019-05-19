#include <Pozyx.h>
#include <Pozyx_definitions.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#define FILTER_TYPE_NONE            0x0
#define FILTER_TYPE_FIR             0x1
#define FILTER_TYPE_MOVING_AVERAGE  0x3
#define FILTER_TYPE_MOVING_MEDIAN   0x4

#define ID_NAV "POS"          // message ID for location data
#define ID_MAG "DIR"          // message ID for course data

/******************
     Pozyx data
*******************/
// TODO check if all data has correct units for fc navigation
typedef struct __attribute__((packed))_pozyx_data {
  magnetic_t magnetic;
  coordinates_t linear_velocity;        // cm/s
  unsigned long last_time;              // ms
  coordinates_t coordinates;            // mm
  uint16_t source_id;                   // the network id of the connected device, will be set automatically
  //pos_error_t pos_error;                // x|y|z variance and xy|xz|yz covariance
} pozyx_data_t;

volatile pozyx_data_t pozyx_data;

//#define DEBUG
#define USE_POZYX

#ifndef USE_POZYX
pozyx_data.coordinates.x = pozyx_data.coordinates.y = 0;  // mm
pozyx_data.coordinates.z = 0;                             // mm

//pozyx_data.angular_vel.x = pozyx_data.angular_vel.y = pozyx_data.angular_vel.z = 0; // degrees per second
pozyx_data.magnetic.x = pozyx_data.magnetic.y = pozyx_data.magnetic.z = 0;  // µT
#endif

SoftwareSerial msp(A1, A2); // rx/tx

unsigned long t_gps;
unsigned long t_mag;

String inputString = "";            // a string to hold incoming data
boolean stringComplete = false;     // whether the string is complete

// FILTER_TYPE_NONE | FILTER_TYPE_MOVING_AVERAGE | FILTER_TYPE_MOVING_MEDIAN |
// FILTER_TYPE_:
//  NONE (Default value). No additional filtering is applied.
//  FIR. A low-pass filter is applied which filters out high-frequency jitters.
//  MOVING_AVERAGE. A moving average filter is applied, which smoothens the trajectory.
//  MOVING_MEDIAN. A moving median filter is applied, which filters out outliers

//        #############################################
//        ######### APPLY TAG PARAMETERS HERE #########
//        #############################################

// TODO compass should be updated at 10hz ?
// TODO gps should be updated at 18hz?
const unsigned int GPS_INTERVAL = 1;      // every 250ms
const unsigned int MAG_INTERVAL = 1;     // every  30ms

const uint8_t filter = FILTER_TYPE_MOVING_MEDIAN;
const uint8_t filter_strength = 8;  // 0-15

const uint8_t num_anchors = 5;                                      // the number of anchors
uint16_t anchors[num_anchors] = {0x6951, 0x6E59, 0x695D, 0x690B, 0x6748};   // network ids of the anchors
int32_t anchors_x[num_anchors] = {0, 5340, 6812, -541, 6812};            // anchor x-coorindates in mm
int32_t anchors_y[num_anchors] = {0, 0, -8923, -10979, -4581};            // anchor y-coordinates in mm
int32_t heights[num_anchors] = {1500, 2000, 2500, 3000, 200};           // anchor z-coordinates in mm


// UWB settings:
//    FAST(100ms bootup required): 110kbps, preamble length 1024 -> 51Hz
//    PRECISION:                   110kbps, preamble length 1024 -> 18Hz

uint8_t algorithm = POZYX_POS_ALG_TRACKING;             // try POZYX_POS_ALG_TRACKING for fast moving objects. (POZYX_POS_ALG_TRACKING|POZYX_POS_ALG_UWB_ONLY)
uint8_t dimension = POZYX_3D;                           // positioning dimension
int32_t height = 1000;

//        #############################################
//        ######### APPLY TAG PARAMETERS HERE #########
//        #############################################


void setup() {
  Serial.begin(115200); // 115200 mag and gps baudrate
  msp.begin(9600);      // 9600 softserial baudrate for msp
  while (!Serial);
  while (!msp);

  // setup time variables for gps and mag messages
  t_gps = 0;
  t_mag = 0;

  pozyx_data.last_time = 0;

#ifdef USE_POZYX

#ifdef DEBUG
  Serial.println("-   INIT POZYX   -");
#endif

  // possible interrupts (do bit-wise combinations):
  //    POZYX_INT_MASK_ERR, POZYX_INT_MASK_POS, POZYX_INT_MASK_IMU, POZYX_INT_MASK_RX_DATA, POZYX_INT_MASK_FUNC.
  //    POZYX_INT_MASK_ALL to trigger on all events.
  while (Pozyx.begin(false, MODE_INTERRUPT, POZYX_INT_MASK_ALL, 0) == POZYX_FAILURE) {

#ifdef DEBUG
    Serial.println("ERROR: Unable to connect to POZYX shield");
    Serial.flush();
#endif
    delay(1000);
  }

#endif

#ifdef USE_POZYX
  // read the network id of this device
  Pozyx.regRead(POZYX_NETWORK_ID, (uint8_t*)&pozyx_data.source_id, 2);

#ifdef DEBUG
  Serial.print("Source ID: "); Serial.println(pozyx_data.source_id, HEX);
#endif

  // clear all previous devices in the device list
  Pozyx.clearDevices(pozyx_data.source_id);
  // sets the anchor manually
  setAnchorsManual();

  // sets the positioning algorithm
  Pozyx.setPositionAlgorithm(algorithm, dimension, pozyx_data.source_id);

  // apply positioning filter and strength
  Pozyx.setPositionFilter(filter, filter_strength);

  // set update intervall for continuous mode
  // TODO is doPositioning still needed?
  Pozyx.setUpdateInterval(101); //100ms-60000ms
#endif

#ifdef DEBUG
  Serial.println(F("Starting positioning: "));
#endif
  Serial.flush();
#ifdef USE_POZYX
  delay(1500);
#endif

}

void loop() {

  unsigned long currentTime = millis();
  int status = 0;


#ifdef USE_POZYX
  // possible status flags:
  //    POZYX_INT_STATUS_ERR, POZYX_INT_STATUS_POS, POZYX_INT_STATUS_IMU,
  //    POZYX_INT_STATUS_RX_DATA, POZYX_INT_STATUS_FUNC

  /*if(Pozyx.waitForFlag(POZYX_INT_STATUS_ERR, 1)) {
    printErrorCode("");
  }*/

  if (Pozyx.waitForFlag(POZYX_INT_STATUS_IMU, 1)) {
    // update new magnetic and vel data
    Pozyx.getMagnetic_uT(&pozyx_data.magnetic);
    linear_acceleration_t accel;
    Pozyx.getLinearAcceleration_mg(&accel);
    
    if(pozyx_data.last_time != 0) {
      // calculate linear velocity
      unsigned long delta_t = currentTime - pozyx_data.last_time;
      
      pozyx_data.linear_velocity.x = accel.x * delta_t / 10; // cm/s
      pozyx_data.linear_velocity.y = accel.y * delta_t / 10; // cm/s
      pozyx_data.linear_velocity.z = accel.z * delta_t / 10; // cm/s
    }
    pozyx_data.last_time = currentTime;

  }

  if (Pozyx.waitForFlag(POZYX_INT_STATUS_POS, 1)) {
    Pozyx.getCoordinates(&pozyx_data.coordinates);
    //Pozyx.getPositionError(&pozyx_data.pos_error);
    // TODO caluclate dilution of precision from pos error
  }

  if (Pozyx.waitForFlag(POZYX_INT_STATUS_RX_DATA, 1))
    // we have received a message via pozyx
    forwardMsgToFC();
#endif

  if (currentTime - t_gps >= GPS_INTERVAL) {
    // time to send position
    forwardNavigation(millis());
  }

  if (currentTime - t_mag >= MAG_INTERVAL) {
    // time to send orientation
    forwardOrientation(millis());
  }

  while (msp.available()) {
    // get msp msg from fc
    char inChar = (char)msp.read();

    // TODO check when msp message ends (probably not '\n')
    if (inChar == '\n') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }

#ifdef DEBUG
    Serial.print("incoming msp msg: "); Serial.println(inputString);
#endif
  }

  if (stringComplete) {
    forwardMsgToPOZYX();
  }
}

// error printing function for debugging
void printErrorCode(String operation) {
  uint8_t error_code;
  if (pozyx_data.source_id == NULL) {
    Pozyx.getErrorCode(&error_code);
    Serial.print("ERROR ");
    Serial.print(operation);
    Serial.print(", local error code: 0x");
    Serial.println(error_code, HEX);
    return;
  }
  int status = Pozyx.getErrorCode(&error_code, pozyx_data.source_id);
  if (status == POZYX_SUCCESS) {
    Serial.print("ERROR ");
    Serial.print(operation);
    Serial.print(" on ID 0x");
    Serial.print(pozyx_data.source_id, HEX);
    Serial.print(", error code: 0x");
    Serial.println(error_code, HEX);
  } else {
    Pozyx.getErrorCode(&error_code);
    Serial.print("ERROR ");
    Serial.print(operation);
    Serial.print(", couldn't retrieve remote error code, local error: 0x");
    Serial.println(error_code, HEX);
  }
}

// function to manually set the anchor coordinates
void setAnchorsManual() {
  for (int i = 0; i < num_anchors; i++) {
    device_coordinates_t anchor;
    anchor.network_id = anchors[i];
    anchor.flag = 0x1;
    anchor.pos.x = anchors_x[i];
    anchor.pos.y = anchors_y[i];
    anchor.pos.z = heights[i];
    Pozyx.addDevice(anchor, pozyx_data.source_id);
  }
  if (num_anchors > 4) {
    Pozyx.setSelectionOfAnchors(POZYX_ANCHOR_SEL_AUTO, num_anchors, pozyx_data.source_id);
  }
}

// format time: hhmmss.sss
String formatTime(unsigned long t) {
  int dd = t / 86400000;
  int hh = (t % 86400000) / 3600000;
  int mm = ((t % 86400000) % 3600000) / 60000;
  double ss = (((t % 86400000) % 3600000) % 60000) / 1000.0;

  String h = (hh > 9 ? String(hh) : "0" + String(hh));
  String m = (mm > 9 ? String(mm) : "0" + String(mm));
  String s = (ss > 9 ? String(ss, 3) : "0" + String(ss, 3));
  return h + m + s;
}

// NMEA CRC: XOR each byte with previous for all chars between '$' and '*'
String calcCRC(char* buff, byte buff_len) {
  char c;
  byte i;
  byte start_with = 0;
  byte end_with = 0;
  byte crc = 0;

  for (i = 0; i < buff_len; i++) {
    c = buff[i];
    if (c == '$') start_with = i;
    else if (c == '*') end_with = i;

  }
  if (end_with > start_with) {
    for (i = start_with + 1; i < end_with; i++) {
      crc = crc ^ buff[i];  // XOR every character between '$' and '*'
    }
  } else {
#ifdef DEBUG
    Serial.println("CRC ERROR");
#endif
    return "-1";
  }
  return String(crc, HEX);
}

void forwardMsgToPOZYX() {

#ifdef DEBUG
  Serial.print("Ox");
  Serial.print(pozyx_data.source_id, HEX);
  Serial.print(": ");
  Serial.println(inputString);
#endif

  int length = inputString.length();
  uint8_t buffer[length];
  inputString.getBytes(buffer, length);

  // write the message to the transmit (TX) buffer
  int status = Pozyx.writeTXBufferData(buffer, length);
  // broadcast the contents of the TX buffer
  status = Pozyx.sendTXBufferData(0);

  inputString = "";
  stringComplete = false;
}

void forwardMsgToFC() {
  uint8_t msg_length = 0;
  uint16_t messenger = 0x00;
  delay(1);

  // Let's read out some information about the message (i.e., how many bytes did we receive and who sent the message)
  Pozyx.getLastDataLength(&msg_length);
  Pozyx.getLastNetworkId(&messenger);
  char data[msg_length];

  // read the contents of the receive (RX) buffer, this is the message that was sent to this device
  // TODO: what if msg is greater than pozyx buffer?!
  Pozyx.readRXBufferData((uint8_t *) data, msg_length);

#ifdef DEBUG
  Serial.print("Ox");
  Serial.print(messenger, HEX);
  Serial.print(": ");
  Serial.print(data);
#endif

  // send data over uart to fc
  msp.write(data, msg_length);
}

void forwardNavigation(unsigned long currentTime) {

  t_gps = millis();

  String t = formatTime(currentTime);
  String date = "011219";

  String str = "$"
               + String(ID_NAV) + ","
               + t + ","
               + date + ","
               + pozyx_data.coordinates.x + ","
               + pozyx_data.coordinates.y + ","
               + pozyx_data.coordinates.z + ","
               + "*";
  byte len = str.length() + 1;
  char buff[len];

  // TODO: fix String->Char, Char->String conversions!
  str.toCharArray(buff, len);
  String gps_msg = str + calcCRC(buff, sizeof(buff));

  Serial.println(gps_msg);
}

void forwardOrientation(unsigned long currentTime) {
  t_mag = millis();

  //example msg: $GPRMC , 161229.487,A,3723.2475,N,12158.3416,W,0.13,309.62,120598, ,*10

  String str = "$"
               + String(ID_MAG) + ","
               + pozyx_data.magnetic.x + ","
               + pozyx_data.magnetic.y + ","
               + pozyx_data.magnetic.z + ","
               + pozyx_data.linear_velocity.x + ","
               + pozyx_data.linear_velocity.y + ","
               + pozyx_data.linear_velocity.z + ","
               + "*";
  byte len = str.length() + 1;
  char buff[len];

  // TODO: fix String->Char, Char->String conversions!
  str.toCharArray(buff, len);
  String mag_msg = str + calcCRC(buff, sizeof(buff));
  Serial.println(mag_msg);
}
