#include <Pozyx.h>
#include <Pozyx_definitions.h>
#include <Wire.h>
#include <SoftwareSerial.h>

//#define DEBUG
#define USE_POZYX

#ifndef USE_POZYX
  #define X 0     // cm
  #define Y 0     // cm
  #define Z 500   // cm
#endif

SoftwareSerial msp(A1, A2); // rx/tx

unsigned long t_gps;
unsigned long t_mag;

const unsigned int GPS_INTERVAL       = 250;      // every 250ms
const unsigned int MAG_INTERVAL       = 30;       // every  30ms

// TODO: use numbers instead of strings!
static const String ID_LOCATION       = "GPS";          // message ID for location data
static const String ID_COURSE         = "MAG";          // message ID for course data

uint16_t source_id;                                     // the network id of the connected device
sensor_raw_t sensor_raw;

//        #############################################
//        ######### APPLY TAG PARAMETERS HERE #########
//        #############################################

const uint8_t num_anchors = 4;                          // the number of anchors
uint16_t anchors[num_anchors] = {0x6951, 0x6E59, 0x695D, 0x690B};  // the network ids of the anchors: change these to the network ids of your anchors
int32_t anchors_x[num_anchors] = {0,5340,6812,-541};    // anchor x-coorindates in mm
int32_t anchors_y[num_anchors] = {0,0,-8923,-10979};    // anchor y-coordinates in mm
int32_t heights[num_anchors] = {1500, 2000, 2500, 3000};// anchor z-coordinates in mm

uint8_t algorithm = POZYX_POS_ALG_UWB_ONLY;             // positioning algorithm to use. try POZYX_POS_ALG_TRACKING for fast moving objects.
uint8_t dimension = POZYX_3D;                           // positioning dimension
int32_t height = 1000;                                  // height of device, required in 2.5D positioning

//        #############################################
//        ######### APPLY TAG PARAMETERS HERE #########
//        #############################################


void setup() {
  Serial.begin(115200); // 115200 mag and gps baudrate
  msp.begin(9600);      // 9600 softserial baudrate for msp
  while(!Serial);
  while(!msp);
  delay(2000); // wait for pozyx to power up

  // setup time variables for gps and mag messages
  t_gps = 0;
  t_mag = 0;

#ifdef USE_POZYX
  #ifdef DEBUG
    Serial.println("-   INIT POZYX   -");
  #endif
  while(Pozyx.begin(false, MODE_INTERRUPT) == POZYX_FAILURE){
#ifdef DEBUG
    Serial.println("ERROR: Unable to connect to POZYX shield");
    Serial.flush();
#endif
    delay(1000);
  }
#endif

#ifdef USE_POZYX
  // read the network id of this device
  Pozyx.regRead(POZYX_NETWORK_ID, (uint8_t*)&source_id, 2);

#ifdef DEBUG
  Serial.print("Source ID: ");Serial.println(source_id, HEX);
#endif
  
  // clear all previous devices in the device list
  Pozyx.clearDevices(source_id);
  // sets the anchor manually
  setAnchorsManual();
  // sets the positioning algorithm
  Pozyx.setPositionAlgorithm(algorithm, dimension, NULL);
#endif

  Serial.flush();
#ifdef USE_POZYX
  delay(2000);
#endif

#ifdef DEBUG
  Serial.println(F("Starting positioning: "));
#endif

}

void loop() {

#ifdef USE_POZYX
  // TODO wait only 1ms or even 0?
  // we wait up to 2ms to see if we have received an incoming message (if so we receive an RX_DATA interrupt)
  if(Pozyx.waitForFlag(POZYX_INT_STATUS_RX_DATA,2))
    // we have received a message!
    forwardMsg();
#endif

  unsigned long currentTime = millis();

  if(currentTime - t_gps >= GPS_INTERVAL)
    // time to send position
    forwardPosition(currentTime);

  currentTime = millis();

  if(currentTime - t_mag >= MAG_INTERVAL)
    // time to send orientation
    forwardOrientation(currentTime);
}



// prints the coordinates for either humans or for processing
void printCoordinates(coordinates_t coor){
  uint16_t network_id = source_id;
  if (network_id == NULL){
    network_id = 0;
  }
  
  Serial.print("POS,0x");
  Serial.print(network_id,HEX);
  Serial.print(",");
  Serial.print(coor.x);
  Serial.print(",");
  Serial.print(coor.y);
  Serial.print(",");
  Serial.println(coor.z);
}

// error printing function for debugging
void printErrorCode(String operation){
  uint8_t error_code;
  if (source_id == NULL){
    Pozyx.getErrorCode(&error_code);
    Serial.print("ERROR ");
    Serial.print(operation);
    Serial.print(", local error code: 0x");
    Serial.println(error_code, HEX);
    return;
  }
  int status = Pozyx.getErrorCode(&error_code, source_id);
  if(status == POZYX_SUCCESS){
    Serial.print("ERROR ");
    Serial.print(operation);
    Serial.print(" on ID 0x");
    Serial.print(source_id, HEX);
    Serial.print(", error code: 0x");
    Serial.println(error_code, HEX);
  }else{
    Pozyx.getErrorCode(&error_code);
    Serial.print("ERROR ");
    Serial.print(operation);
    Serial.print(", couldn't retrieve remote error code, local error: 0x");
    Serial.println(error_code, HEX);
  }
}

// function to manually set the anchor coordinates
void setAnchorsManual(){
  for(int i = 0; i < num_anchors; i++){
    device_coordinates_t anchor;
    anchor.network_id = anchors[i];
    anchor.flag = 0x1;
    anchor.pos.x = anchors_x[i];
    anchor.pos.y = anchors_y[i];
    anchor.pos.z = heights[i];
    Pozyx.addDevice(anchor, source_id);
  }
  if (num_anchors > 4){
    Pozyx.setSelectionOfAnchors(POZYX_ANCHOR_SEL_AUTO, num_anchors, source_id);
  }
}

String genMagMsg(float groundSpeed, unsigned long t) {
  //example msg: $GPRMC , 161229.487,A,3723.2475,N,12158.3416,W,0.13,309.62,120598, ,*10

#ifdef USE_POZYX
  Pozyx.getRawSensorData(&sensor_raw);
  float heading = atan2(sensor_raw.magnetic[1], sensor_raw.magnetic[0]);
  float declinationAngle = 0.047; // declination angle 2°46' for Kassel
  heading += declinationAngle;
  
  // Correct for when signs are reversed.
  if(heading < 0)
    heading += 2*PI;
    
  // Check for wrap due to addition of declination.
  if(heading > 2*PI)
    heading -= 2*PI;
   
  // Convert radians to degrees for readability.
  float groundCourse = heading * 180/M_PI;
#else
  float groundCourse = 45.0;
#endif
  
  String tt = formatTime(t);
  String date = "011219";
  String str = "$"
        + ID_COURSE+","
        + tt+","
        + groundSpeed+","
        + groundCourse+","
        + date+","
        + "*";
  byte len = str.length()+1;
  char buff[len];

  // TODO: fix String->Char, Char->String conversions!
  str.toCharArray(buff, len);
  String msg = str + calcCRC(buff, sizeof(buff));
  return msg;
}

// prepares char array for Serial communication
String genGpsMsg(int x, int y, int z, unsigned long t) {
  String tt = formatTime(t);

  char x_sign = '+';
  char y_sign = '+';
  char z_sign = '+';

  if(x < 0) {
    x = -x;
    x_sign = '-';
  }

  if(y < 0) {
    y = -y;
    y_sign = '-';
  }

  if(z < 0) {
    z = -z;
    z_sign = '-';
  }

  String str = "$"
        + ID_LOCATION+","
        + tt+","
        + x+","
        + x_sign+","
        + y+","
        + y_sign+","
        + z+","
        + z_sign
        + "*";
  byte len = str.length()+1;
  char buff[len];

  // TODO: fix String->Char, Char->String conversions!
  str.toCharArray(buff, len);
  String msg = str + calcCRC(buff, sizeof(buff));
  return msg;
}

String genGpsMsg(double x, double y, double altitude) {
  return genGpsMsg(x, y, altitude, millis());
}

// format time: hhmmss.sss
String formatTime(unsigned long t) {
  int dd = t / 86400000;
  int hh = (t % 86400000) / 3600000;
  int mm = ((t % 86400000) % 3600000) / 60000;
  double ss = (((t % 86400000) % 3600000) % 60000) / 1000.0;

  String h = (hh > 9 ? String(hh) : "0" + String(hh));
  String m = (mm > 9 ? String(mm) : "0" + String(mm));
  String s = (ss > 9 ? String(ss,3) : "0" + String(ss,3));
  return h+m+s;
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
    if(c == '$') start_with = i;
    else if(c == '*') end_with = i;

  }
  if (end_with > start_with){
    for (i = start_with+1; i < end_with; i++){
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

void forwardMsg() {
  uint8_t msg_length = 0;
  uint16_t messenger = 0x00;
  delay(1);
  
  // Let's read out some information about the message (i.e., how many bytes did we receive and who sent the message)
  Pozyx.getLastDataLength(&msg_length);
  Pozyx.getLastNetworkId(&messenger);
  uint8_t data[msg_length];

  // read the contents of the receive (RX) buffer, this is the message that was sent to this device
  // TODO: what if msg is greater than pozyx buffer?! 24byte?
  Pozyx.readRXBufferData((uint8_t *) data, msg_length);

#ifdef DEBUG
  Serial.print("Ox");
  Serial.print(messenger, HEX);
  Serial.print(": ");
#endif

  // send data over uart to fc
  // TODO use softserial port and send only msp messages over this port instead of gps, mag and msp. check if fc has another empty port
  msp.write(data, sizeof(data)/sizeof(data[0]));
}

void forwardPosition(unsigned long currentTime) {
  coordinates_t position;
  
#ifdef USE_POZYX
  int coordinates[3] = {position.x, position.y, position.z};
  int status = Pozyx.doPositioning(&position, dimension, height, algorithm);
#ifdef DEBUG
  if (status == POZYX_SUCCESS){
    // prints out the result
    printCoordinates(position);
  }else{
    // prints out the error code
    printErrorCode("positioning");
  }
#endif
#else
  int coordinates[3] = {X,Y,Z};
#endif

  t_gps = millis();
  String gps_msg = genGpsMsg(coordinates[0], coordinates[1], coordinates[2], currentTime);
  Serial.println(gps_msg);
}

void forwardOrientation(unsigned long currentTime) {
  t_mag = millis();
  float groundSpeed = 0.0;
  String mag_msg = genMagMsg(groundSpeed, currentTime);
  Serial.println(mag_msg);
}
