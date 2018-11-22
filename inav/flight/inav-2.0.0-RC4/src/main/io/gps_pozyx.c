/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "common/typeconversion.h"
#include "platform.h"

#if defined(USE_GPS) && defined(USE_GPS_PROTO_POZYX)

#include "build/build_config.h"
#include "build/debug.h"

#include "common/axis.h"
#include "common/gps_conversion.h"
#include "common/maths.h"
#include "common/utils.h"

#include "drivers/serial.h"
#include "drivers/time.h"

#include "fc/config.h"
#include "fc/runtime_config.h"

#include "io/serial.h"
#include "io/gps.h"
#include "io/gps_private.h"

/* This is a light implementation of a POZYX frame decoding
   It assumes there are some POZYX frames to decode on the serial bus
   Now verifies checksum correctly before applying data

   Here we use only the following data :
     - x
     - y
     - z
*/

#define NO_FRAME        0
#define FRAME_LOCATION  1
#define FRAME_ANCHOR    2
#define FRAME_M_START   3
#define FRAME_M_STOP    4
#define FRAME_WP_ADD    5
#define FRAME_WP_DEL    6

static uint32_t grab_fields(char *src, uint8_t mult)
{                               // convert string to uint32
    uint32_t i;
    uint32_t tmp = 0;
    for (i = 0; src[i] != 0; i++) {
        if (src[i] == '.') {
            i++;
            if (mult == 0)
                break;
            else
                src[i + mult] = 0;
        }
        tmp *= 10;
        if (src[i] >= '0' && src[i] <= '9')
            tmp += src[i] - '0';
        if (i >= 15)
            return 0; // out of bounds
    }
    return tmp;
}

#define POZYX_BUFFER_SIZE        16

static bool gpsNewFramePOZYX(char c)
{
    static gpsDataPozyx_t gps_Msg;

    uint8_t frameOK = 0;
    static uint8_t param = 0, offset = 0, parity = 0;
    static char string[POZYX_BUFFER_SIZE];
    static uint8_t checksum_param, gps_frame = NO_FRAME;

    switch (c) {
        case '$':
            param = 0;
            offset = 0;
            parity = 0;
            break;
        case ',':
        case '*':
            string[offset] = 0;
            if (param == 0) {       //frame identification
                gps_frame = NO_FRAME;
                if (strcmp(string, "POZYX") == 0) {
                    gps_frame = FRAME_LOCATION;
                } else if(strcmp(string, "WP_ADD") == 0) {
                    gps_frame = FRAME_WP_ADD;
                }
                // TODO[uniks] add anchor init frame wich sets anchor position X,Y,Z
            }

            switch (gps_frame) {
                //************* POZYX LOCATION FRAME parsing *************
                case FRAME_LOCATION:
                    switch (param) {
                        case 1:             // Time information
                            break;
                        case 2:
                            gps_Msg.x = grab_fields(string,8);
                            break;
                        case 3:
                            if (string[0] == '-')
                                gps_Msg.x_sign = -1;
                            else
                                gps_Msg.x_sign = 1;

                            break;
                        case 4:
                            gps_Msg.y = grab_fields(string,8);
                            break;
                        case 5:
                            if (string[0] == '-')
                                gps_Msg.y_sign = -1;
                            else
                                gps_Msg.y_sign = 1;
                            break;
                        case 6:
                            gps_Msg.z = grab_fields(string,8);
                            break;
                        case 7:
                            if(string[0] == '-')
                                gps_Msg.z_sign = -1;
                            else
                                gps_Msg.z_sign = 1;
                            break;
                    }
                    break;
                case FRAME_WP_ADD:
                    break;
                // TODO[uniks]: add all remaining frames
            }

            param++;
            offset = 0;
            if (c == '*')
                checksum_param = 1;
            else
                parity ^= c;
            break;
        case '\r':
        case '\n':
            if (checksum_param) {   //parity checksum
                uint8_t checksum = 16 * ((string[0] >= 'A') ? string[0] - 'A' + 10 : string[0] - '0') + ((string[1] >= 'A') ? string[1] - 'A' + 10 : string[1] - '0');

                if (checksum == parity) {
                    gpsStats.packetCount++;
                    switch (gps_frame) {
                        // TODO[uniks]: also add remaining frames here?
                        case FRAME_LOCATION:
                            frameOK = 1;

                            gpsSol.numSat = 12;
                            gpsSol.fixType = GPS_FIX_3D;

                            cartToSph(&gps_Msg);

                            gpsSol.llh.lat = gps_Msg.latitude;
                            gpsSol.llh.lon = gps_Msg.longitude;
                            gpsSol.llh.alt = gps_Msg.altitude;

                            // TODO[uniks]: keep?
                            gpsSol.llh.x = gps_Msg.x;
                            gpsSol.llh.y = gps_Msg.y;
                            gpsSol.llh.z = gps_Msg.z;
                            // TODO[uniks]: keep?

                            // EPH/EPV are unreliable for NMEA as they are not real accuracy
                            gpsSol.hdop = gpsConstrainHDOP(HDOP_SCALE);
                            gpsSol.eph = gpsConstrainEPE(HDOP_SCALE * GPS_HDOP_TO_EPH_MULTIPLIER);
                            gpsSol.epv = gpsConstrainEPE(HDOP_SCALE * GPS_HDOP_TO_EPH_MULTIPLIER);
                            gpsSol.flags.validEPE = 0;


                            // NMEA does not report VELNED
                            gpsSol.flags.validVelNE = 0;
                            gpsSol.flags.validVelD = 0;
                            break;
                    } // end switch
                } else {
                    gpsStats.errors++;
                }
            }
            checksum_param = 0;
            break;
        default:
            if (offset < (POZYX_BUFFER_SIZE-1)) {    // leave 1 byte to trailing zero
                string[offset++] = c;

                // only checksum if character is recorded and used (will cause checksum failure on dropped characters)
                if (!checksum_param)
                    parity ^= c;
            }
    }

    return frameOK;
}

static bool gpsReceiveData(void)
{
    bool hasNewData = false;

    if (gpsState.gpsPort) {
        while (serialRxBytesWaiting(gpsState.gpsPort)) {
            uint8_t newChar = serialRead(gpsState.gpsPort);
            if (gpsNewFramePOZYX(newChar)) {
                gpsSol.flags.gpsHeartbeat = !gpsSol.flags.gpsHeartbeat;
                gpsSol.flags.validVelNE = 0;
                gpsSol.flags.validVelD = 0;
                hasNewData = true;
            }
        }
    }

    return hasNewData;
}

static bool gpsInitialize(void)
{
    gpsSetState(GPS_CHANGE_BAUD);
    return false;
}

static bool gpsChangeBaud(void)
{
    gpsFinalizeChangeBaud();
    return false;
}

bool gpsHandlePOZYX(void)
{
    // Receive data
    bool hasNewData = gpsReceiveData();

    // Process state
    switch (gpsState.state) {
    default:
        return false;

    case GPS_INITIALIZING:
        return gpsInitialize();

    case GPS_CHANGE_BAUD:
        return gpsChangeBaud();


    case GPS_CHECK_VERSION:
    case GPS_CONFIGURE:
        // No autoconfig, switch straight to receiving data
        gpsSetState(GPS_RECEIVING_DATA);
        return false;

    case GPS_RECEIVING_DATA:
        return hasNewData;
    }
}

#endif
