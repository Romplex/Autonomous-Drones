//#include <Wire.h>
#include "gps.h"
#include "compass.h"

#define DEBUG true

uint8_t buffer[bufferSize];

typedef enum {
    GPS_DATA,
    COMPASS_DATA
} module_state;

enum {
    HEADER1 = 0x55,
    HEADER2 = 0xAA,
    ID_NAV = 0x10,
    ID_MAG = 0x20,
    ID_VER = 0x30,  // not used
    LEN_NAV = 0x3A,
    LEN_MAG = 0x06,
} protocol_bytes;

const long GPS_INTERVAL = 250;
const long COMPASS_INTERVAL = 30;

unsigned long prev_millis_gps = 0;
unsigned long prev_millis_compass = 0;

module_state state;

void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial.println("START");
}

void loop() {
  unsigned long current_millis = millis();
//  if(current_millis - prev_millis_gps >= GPS_INTERVAL) {
//    state = GPS_DATA;
//    prev_millis_gps = current_millis;
//    sendData();
//  }

  if(current_millis - prev_millis_compass >= COMPASS_INTERVAL) {
    state = COMPASS_DATA;
    prev_millis_compass = current_millis;
    sendData();
  }
  delay(1000);
  
}

void sendData() {
    if(state == GPS_DATA) {
      Serial.println(0);
      byte dat[4] = {0x00};                   // date: YYYYYYYMMMMDDDDDHHHHMMMMMMSSSSSS
      byte lon[4] = {0x00, 0x00, 0x00, 0x00}; // x10^7, degree decimal
      byte lat[4] = {0x00, 0x00, 0x00, 0x00}; // x10^7, degree decimal
      byte alt[4] = {0x00, 0x00, 0x00, 0x00}; // altitude in mm
      byte hoa[4] = {0x00, 0x00, 0x00, 0x00}; // horizontal accuracy
      byte vea[4] = {0x00, 0x00, 0x00, 0x00}; // vertical accuracy
      byte reserv = 0x00;                     // reserved
      byte nov[4] = {0x00, 0x00, 0x00, 0x00}; // north velocity
      byte eav[4] = {0x00, 0x00, 0x00, 0x00}; // east velocity
      byte doV[4] = {0x00, 0x00, 0x00, 0x00}; // down velocity
      byte pdp[2] = {0x00, 0x00};             // position dop
      byte vdp[2] = {0x00, 0x00};             // vertical dop
      byte ndp[2] = {0x00, 0x00};             // north dop
      byte edp[2] = {0x00, 0x00};             // east dop
      byte nr_sat = 0x03;                     // nr of satellites
      byte fix_tp = 0x03;                     // fix type: 0 - no lock, 2 - 2D, 3 - 3D
      byte fix_st = 0x00;                     // fix status flag
      byte xr_msk = 0x00;                     // xor mask
      byte seq[2] = {0x00, 0x00};             // sequence nr
      byte cks[2] = {0x00, 0x00};             // checksum
      // ... TODO
      Serial.println(1);
      byte out_nav[10] = {HEADER1, HEADER2, ID_NAV, LEN_NAV, dat[0], dat[1], dat[2], dat[3], lon, lat};
      Serial.println(2);
    } else if(state == COMPASS_DATA) {
      
      byte x[] = {0x00, 0x00};
      byte y[] = {0x00, 0x00};
      byte z[] = {0x00, 0x00};
      
      // 97 DE  2C  20  5A  DE
      byte out_mag[12] = {HEADER1, HEADER2, ID_MAG, LEN_MAG, x[0], x[1], y[0], y[1], z[0], z[1], 0x00, 0x00};
      for(int i = 0; i < 12; i++) {
        //calc checksum
        if(i >= 2 && i < 10) {
          out_mag[10] += out_mag[i];
          out_mag[11] += out_mag[10];
        }
        
#ifndef DEBUG
      Serial.write(out_mag[i]);
#else
      Serial.print(out_mag[i], HEX);
#endif

      }
#ifdef DEBUG
      Serial.println();
#endif
    }
}
