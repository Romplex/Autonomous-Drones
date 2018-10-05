#include "Arduino.h"

// SEE: https://www.rcgroups.com/forums/showpost.php?p=26248426&postcount=62

/* Msg Format COMPASS (little endian):
    55 AA 20 06 CX CX CY CY CZ CZ CS CS

    HEADER
    -------------
    BYTE 1-2: message header - always 55 AA
    BYTE 3: message id (0x20 for compass message)
    BYTE 4: length of the payload (0x06 or 6 decimal for 0x20 message)
    
    PAYLOAD
    --------------
    BYTE 5-6 (CX): compass X axis data (signed) - see comments below
    BYTE 7-8 (CY): compass Y axis data (signed) - see comments below
    BYTE 9-10 (CZ): compass Z axis data (signed) - see comments below
    
    CHECKSUM
    -----------------
    BYTE 11-12 (CS): checksum, calculated the same way as for uBlox binary messages

*/

typedef struct {
    uint16_t x; // 0
    uint16_t y; // 2
    uint16_t z; // 4
} mag;

const uint8_t buffer_size_mag = 12;
