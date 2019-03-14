/**
  Checks distances between the tag and every other tag in range(ancors too).
  TODO: need a filter and function wich should be called if the distance will be to small 
*/

#include <Pozyx.h>
#include <Pozyx_definitions.h>
#include <Wire.h>

////////////////////////////////////////////////
////////////////// PARAMETERS //////////////////
////////////////////////////////////////////////

//uint16_t destination_id = 0x673d;     // the network id of the other pozyx device: fill in the network id of the other device

uint8_t ranging_protocol = POZYX_RANGE_PROTOCOL_PRECISION; // ranging protocol of the Pozyx.

uint16_t remote_id = 0x6760;          // the network ID of the remote device
bool remote = false;                  // whether to use the given remote device for ranging

int prev_list_size = -1;
uint16_t* devices = new uint16_t[0];
uint16_t* prev_devices = new uint16_t[0];

////////////////////////////////////////////////

void setup(){
    Serial.begin(115200);

    if(Pozyx.begin() == POZYX_FAILURE){
        Serial.println("ERROR: Unable to connect to POZYX shield");
        Serial.println("Reset required");
        delay(100);
        abort();
    }
    // setting the remote_id back to NULL will use the local Pozyx
    if (!remote){
        remote_id = NULL;
    }
    Serial.println("multi device ranging online");

    // set the ranging protocol
    Pozyx.setRangingProtocol(ranging_protocol, remote_id);
}

bool deviceListsAreEqual(uint16_t* deviceList1, uint16_t* deviceList2, int list_size) {
    // controlles both device lists are the same
    bool areEqual = true;
    int i = 0;
    while (areEqual && i<list_size) {
        areEqual = listHasDevice(deviceList2, list_size, deviceList1[i]);
        i++;
    }
    return areEqual;
}

bool listHasDevice(uint16_t* deviceList, int list_size, uint16_t device) {
    bool hasDevice = false;
    int i = 0;
    while (!hasDevice && i<list_size) {
        hasDevice = deviceList[i] == device;
        i++;
    }
    return hasDevice;
}

void range(uint16_t destination_id){

  device_range_t range;

    int status = 0;

    // let's perform ranging with the destination
    if(!remote)
        status = Pozyx.doRanging(destination_id, &range);
    else
        status = Pozyx.doRemoteRanging(remote_id, destination_id, &range);

    if (status == POZYX_SUCCESS){
        Serial.print("From: ");
        Serial.print(destination_id,HEX);
        //Serial.print(range.timestamp);
       // Serial.print("ms, ");
        Serial.print("; ");
        Serial.print(range.distance/10);
        Serial.print("cm, ");
        Serial.print(range.RSS);
        Serial.println("dBm");
    }
    else{
        Serial.println("ERROR: ranging");
    }
}

void getDeviceIdsInRange(){

    // Get new device list
    Pozyx.clearDevices();
    int status = Pozyx.doDiscovery(POZYX_DISCOVERY_ALL_DEVICES);
    uint8_t list_size = 0;
    status = Pozyx.getDeviceListSize(&list_size);
    devices = new uint16_t[list_size];

    if (list_size>0)
        status = Pozyx.getDeviceIds(devices, list_size);

    // Check if device list has changed
    bool hasChanged;
    if (prev_list_size != list_size) {
        hasChanged = true;
    } else {
        hasChanged = !deviceListsAreEqual(devices, prev_devices, list_size);
    }

    // Print device list when changed
    if(hasChanged)
    {
        Serial.print("Number of pozyx devices discovered: ");
        Serial.println(list_size);

        if(list_size > 0)
        {
            Serial.println("List of device IDs: ");
            for(int i=0; i<list_size; i++)
            {
                Serial.print("\t0x");
                Serial.println(devices[i], HEX);
            }
        }
    }

    delete[] prev_devices;
    prev_devices = devices;
    prev_list_size = list_size;
}

void loop(){

  getDeviceIdsInRange();
  //range(0x673d);
  uint8_t list_size = 0;
  for( int i=0; i<Pozyx.getDeviceListSize(&list_size);i++){
         range(devices[i]);

  }

  
    
}
