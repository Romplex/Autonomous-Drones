#include <Pozyx.h>
#include <Pozyx_definitions.h>
#include <Wire.h>

#define DEBUG
#define USE_POZYX

#ifndef USE_POZYX
  #define X 0     // cm
  #define Y 0     // cm
  #define Z 500   // cm
#endif

#ifndef DEBUG1
  unsigned long t_gps;
  unsigned long t_mag;
#endif

const unsigned int GPS_INTERVAL       = 250;      // every 250ms
const unsigned int MAG_INTERVAL       = 30;       // every   30ms

// TODO: use numbers instead of strings!
static const String ID_LOCATION       = "GPS";    // message ID for location data
static const String ID_COURSE         = "MAG";    // message ID for course data
static const String ID_ANCHOR         = "ANCHOR"; // message ID for anchor calibration
static const String ID_MISSION_START  = "M_START";// message ID for mission start
static const String ID_MISSION_STOP   = "M_STOP"; // message ID for mission stop
static const String ID_WP_ADD         = "WP_ADD"; // message ID for adding wp to mission
static const String ID_WP_REMOVE      = "WP_DEL"; // message ID for removing wp from mission


//        #############################################
//        ######### APPLY TAG PARAMETERS HERE #########
//        #############################################

uint16_t source_id = 0x6760;                            // set this to the ID of the remote device
uint16_t destination_id = 0;        // the destination network id. 0 means the message is broadcasted to every device in range
String inputString = "";            // a string to hold incoming data
boolean stringComplete = false;     // whether the string is complete

sensor_raw_t sensor_raw;

bool     remote = false;                                // set this to true to use the remote ID

boolean  use_processing = false;                        // set this to true to output data for the processing sketch

const uint8_t num_anchors = 4;                          // the number of anchors
uint16_t anchors[num_anchors] = {0x6951, 0x6E59, 0x695D, 0x690B};     // the network id of the anchors: change these to the network ids of your anchors.
// TODO: measure actual coordinates
int32_t anchors_x[num_anchors] = {0,5340,6812,-541};     // anchor x-coorindates in mm
int32_t anchors_y[num_anchors] = {0,0,-8923,-10979};     // anchor y-coordinates in mm
int32_t heights[num_anchors] = {1500, 2000, 2500, 3000};// anchor z-coordinates in mm

uint8_t algorithm = POZYX_POS_ALG_UWB_ONLY;             // positioning algorithm to use. try POZYX_POS_ALG_TRACKING for fast moving objects.
uint8_t dimension = POZYX_3D;                           // positioning dimension
int32_t height = 1000;                                  // height of device, required in 2.5D positioning

//        #############################################
//        ######### APPLY TAG PARAMETERS HERE #########
//        #############################################


void setup() {
  Serial.begin(115200); // 57600 or 115200
  while(!Serial);
  delay(2000); // wait for pozyx to power up

  // setup time variables for gps and mag messages
  t_gps = 0;
  t_mag = 0;

#ifdef USE_POZYX
  #ifdef DEBUG
    Serial.println("-   INIT POZYX   -");
  #endif
  while(Pozyx.begin() == POZYX_FAILURE){
#ifdef DEBUG
    Serial.println("ERROR: Unable to connect to POZYX shield");
//    Serial.println("Reset required");
    Serial.flush();
#endif
    delay(1000);
//    abort();
  }
#endif

#ifdef DEBUG
  Serial.println(F("----------POZYX POSITIONING V1.1----------"));
  Serial.println(F("NOTES:"));
  Serial.println(F("- No parameters required."));
  Serial.println();
  Serial.println(F("- System will auto start anchor configuration"));
  Serial.println();
  Serial.println(F("- System will auto start positioning"));
  Serial.println(F("----------POZYX POSITIONING V1.1----------"));
  Serial.println();
  Serial.println(F("Performing manual anchor configuration:"));
#endif

#ifdef USE_POZYX
  // read the network id of this device
  Pozyx.regRead(POZYX_NETWORK_ID, (uint8_t*)&source_id, 2);

  // reserve 100 bytes for the inputString:
  inputString.reserve(100);
  
  // clear all previous devices in the device list
  Pozyx.clearDevices(source_id);
  // sets the anchor manually
  setAnchorsManual();
  // sets the positioning algorithm
  Pozyx.setPositionAlgorithm(algorithm, dimension, source_id);
#endif

  // TODO: delay of 2000 needed after flush?
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
// check if we received a newline character and if so, broadcast the inputString.
  if(stringComplete){
    Serial.print("Ox");
    Serial.print(source_id, HEX);
    Serial.print(": ");
    Serial.println(inputString);

    int length = inputString.length();
    uint8_t buffer[length];
    inputString.getBytes(buffer, length);

    // write the message to the transmit (TX) buffer
    int status = Pozyx.writeTXBufferData(buffer, length);
    // broadcast the contents of the TX buffer
    status = Pozyx.sendTXBufferData(destination_id);

    inputString = "";
    stringComplete = false;
  }

  // we wait up to 50ms to see if we have received an incoming message (if so we receive an RX_DATA interrupt)
  if(Pozyx.waitForFlag(POZYX_INT_STATUS_RX_DATA,50))
  {
    // we have received a message!

    uint8_t length = 0;
    uint16_t messenger = 0x00;
    delay(1);
    // Let's read out some information about the message (i.e., how many bytes did we receive and who sent the message)
    Pozyx.getLastDataLength(&length);
    Pozyx.getLastNetworkId(&messenger);

    char data[length];

    // read the contents of the receive (RX) buffer, this is the message that was sent to this device
    Pozyx.readRXBufferData((uint8_t *) data, length);
    Serial.print("Ox");
    Serial.print(messenger, HEX);
    Serial.print(": ");
    Serial.println(data);
  }
// -----------------------------------------------------------------------------------

  coordinates_t position;
  int status;
  if(remote){
    status = Pozyx.doRemotePositioning(source_id, &position, dimension, height, algorithm);
  }else{
    status = Pozyx.doPositioning(&position, dimension, height, algorithm);
  }
#endif

#if defined(DEBUG1) && defined(USE_POZYX)
  if (status == POZYX_SUCCESS){
    // prints out the result
    printCoordinates(position);
  }else{
    // prints out the error code
    printErrorCode("positioning");
  }
#endif

#ifdef DEBUG1
  Serial.print("x: "); Serial.println(position.x);
  Serial.print("y: "); Serial.println(position.y);
  Serial.print("z: "); Serial.println(position.z);
#endif

#ifdef USE_POZYX
  int coordinates[3] = {position.x, position.y, position.z};
#else
  int coordinates[3] = {X,Y,Z};
#endif

  String gps_msg;
  String mag_msg;

  long currentTime = millis();
  
  if(currentTime - t_gps >= GPS_INTERVAL) {
    t_gps = millis();
    gps_msg = genGpsMsg(coordinates[0], coordinates[1], coordinates[2], currentTime);
    Serial.println(gps_msg);
  }

  currentTime = millis();

  if(currentTime - t_mag >= MAG_INTERVAL) {
    t_mag = millis();
    mag_msg = genMagMsg(0.0, currentTime);
    Serial.println(mag_msg);
  }
}



// prints the coordinates for either humans or for processing
void printCoordinates(coordinates_t coor){
  uint16_t network_id = source_id;
  if (network_id == NULL){
    network_id = 0;
  }
  if(!use_processing){
    Serial.print("POS ID 0x");
    Serial.print(network_id, HEX);
    Serial.print(", x(mm): ");
    Serial.print(coor.x);
    Serial.print(", y(mm): ");
    Serial.print(coor.y);
    Serial.print(", z(mm): ");
    Serial.println(coor.z);
  }else{
    Serial.print("POS,0x");
    Serial.print(network_id,HEX);
    Serial.print(",");
    Serial.print(coor.x);
    Serial.print(",");
    Serial.print(coor.y);
    Serial.print(",");
    Serial.println(coor.z);
  }
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

// print out the anchor coordinates
void printCalibrationResult(){
  uint8_t list_size;
  int status;

  status = Pozyx.getDeviceListSize(&list_size, source_id);
  Serial.print("list size: ");
  Serial.println(status*list_size);

  if(list_size == 0){
    printErrorCode("configuration");
    return;
  }

  uint16_t device_ids[list_size];
  status &= Pozyx.getDeviceIds(device_ids, list_size, source_id);

  Serial.println(F("Calibration result:"));
  Serial.print(F("Anchors found: "));
  Serial.println(list_size);

  coordinates_t anchor_coor;
  for(int i = 0; i < list_size; i++)
  {
    Serial.print("ANCHOR,");
    Serial.print("0x");
    Serial.print(device_ids[i], HEX);
    Serial.print(",");
    Pozyx.getDeviceCoordinates(device_ids[i], &anchor_coor, source_id);
    Serial.print(anchor_coor.x);
    Serial.print(",");
    Serial.print(anchor_coor.y);
    Serial.print(",");
    Serial.println(anchor_coor.z);
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
  //$GPRMC , 161229.487,A,3723.2475,N,12158.3416,W,0.13,309.62,120598, ,*10

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

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();

    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it.
    // otherwise, add it to the inputString:
    if (inChar == '\n') {
      stringComplete = true;
    }else{
      inputString += inChar;
    }
  }
}
