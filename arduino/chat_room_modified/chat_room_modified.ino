/**
  The pozyx chat demo
  please check out https://www.pozyx.io/Documentation/Tutorials/getting_started

  This demo requires at least two pozyx shields and an equal number of Arduino's.
  It demonstrates the wireless messaging capabilities of the pozyx device.

  This demo creates a chat room. Text written in the Serial monitor will be broadcasted to all other pozyx devices
  within range. They will see your message appear in their Serial monitor.
*/

#include <Pozyx.h>
#include <Pozyx_definitions.h>
#include <Wire.h>

uint16_t source_id;                 // the network id of this device
uint16_t destination_id = 0;        // the destination network id. 0 means the message is broadcasted to every device in range
uint8_t waypointLength = 27;
uint8_t waypoint[27];


void setup() {
  Serial.begin(115200);

  // initialize Pozyx
  if (! Pozyx.begin(false, MODE_INTERRUPT, POZYX_INT_MASK_RX_DATA, 0)) {
    Serial.println("ERROR: Unable to connect to POZYX shield");
    Serial.println("Reset required");
    abort();
  }

  // read the network id of this device
  Pozyx.regRead(POZYX_NETWORK_ID, (uint8_t*)&source_id, 2);

  // reserve 100 bytes for the inputString:
  Serial.println("--- Pozyx Chat started ---");
}

void loop() {
  uint8_t i = 0;
  Serial.println("Start construction");
  while (i < waypointLength) {
    if (Pozyx.waitForFlag(POZYX_INT_STATUS_RX_DATA, 50)) {
      Serial.println("RECEIVED!");
      uint8_t length = 0;
      delay(1);
      Pozyx.getLastDataLength(&length);
      char data[length];
      Pozyx.readRXBufferData((uint8_t *) data, length);
      uint8_t d = charToUInt8(data, length);
      Serial.println(d);
      waypoint[i] = d;
      i++;
    }
  }

  Serial.print("WAYPOINT: ");
  Serial.print("[");
  for (uint8_t i = 0; i < waypointLength; ++i) {
    Serial.print(waypoint[i]);
    if (i < waypointLength - 1) {
      Serial.print(", ");
    }
  }
  Serial.println("]");
}

uint8_t charToUInt8(char* data, uint8_t length) {
  String result = "";
  for (uint8_t i = 0; i < length; ++i) {
    result += data[i];
  }
  return result.toInt();
}
