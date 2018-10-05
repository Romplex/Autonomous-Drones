#include "Arduino.h"

// SEE: https://www.rcgroups.com/forums/showpost.php?p=26210591&postcount=15

/* Msg Format GPS (little endian):
    55 AA 10 3A DT DT DT DT LO LO LO LO LA LA LA LA AL AL AL AL HA HA HA HA VA VA VA VA XX XX XX XX NV NV NV NV EV EV EV EV DV DV DV DV PD PD VD VD ND ND ED ED NS XX FT XX SF XX XX XM SN SN CS CS

    HEADER
    -------------
    BYTE 1-2: message header - always 55 AA
    BYTE 3: message id (0x10 for GPS message)
    BYTE 4: lenght of the payload (0x3A or 58 decimal for 0x10 message)
    
    PAYLOAD
    --------------
    BYTE 5-8 (DT): date and time, see details below
    BYTE 9-12 (LO): longitude (x10^7, degree decimal)
    BYTE 13-16 (LA): latitude (x10^7, degree decimal)
    BYTE 17-20 (AL): altitude (in milimeters)
    BYTE 21-24 (HA): horizontal accuracy estimate (see uBlox NAV-POSLLH message for details)
    BYTE 25-28 (VA): vertical accuracy estimate (see uBlox NAV-POSLLH message for details)
    BYTE 29-32: ??? (seems to be always 0)
    BYTE 33-36 (NV): NED north velocity (see uBlox NAV-VELNED message for details)
    BYTE 37-40 (EV): NED east velocity (see uBlox NAV-VELNED message for details)
    BYTE 41-44 (DV): NED down velocity (see uBlox NAV-VELNED message for details)
    BYTE 45-46 (PD): position DOP (see uBlox NAV-DOP message for details)
    BYTE 47-48 (VD): vertical DOP (see uBlox NAV-DOP message for details)
    BYTE 49-50 (ND): northing DOP (see uBlox NAV-DOP message for details)
    BYTE 51-52 (ED): easting DOP (see uBlox NAV-DOP message for details)
    BYTE 53 (NS): number of satellites (not XORed)
    BYTE 54: ??? (not XORed, seems to be always 0)
    BYTE 55 (FT): fix type (0 - no lock, 2 - 2D lock, 3 - 3D lock, not sure if other values can be expected - see uBlox NAV-SOL message for details)
    BYTE 56: ??? (seems to be always 0)
    BYTE 57 (SF): fix status flags (see uBlox NAV-SOL message for details)
    BYTE 58-59: ??? (seems to be always 0)
    BYTE 60 (XM): not sure yet, but I use it as the XOR mask
    BYTE 61-62 (SN): sequence number (not XORed), once there is a lock - increases with every message. When the lock is lost later LSB and MSB are swapped with every message.
    
    CHECKSUM
    -----------------
    BYTE 63-64 (CS): checksum, calculated the same way as for uBlox binary messages
*/

typedef struct {
    uint32_t time; // GPS msToW 0
    int32_t longitude; // 4
    int32_t latitude; // 8
    int32_t altitude_msl; // 12
    int32_t h_acc; // 16
    int32_t v_acc; // 20
    int32_t reserved;
    int32_t ned_north; // 28
    int32_t ned_east; // 32
    int32_t ned_down;  // 36
    uint16_t pdop;  // 40
    uint16_t vdop;  // 42
    uint16_t ndop; // 44
    uint16_t edop;  // 46
    uint8_t satellites; // 48
    uint8_t reserved3; //
    uint8_t fix_type; // 50
    uint8_t reserved4; //
    uint8_t fix_status; // 52
    uint8_t reserved5;
    uint8_t reserved6;
    uint8_t mask;   // 55
} nav;

const uint8_t buffer_size_nav = 64;
