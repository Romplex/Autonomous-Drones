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

#include "io/gps.h"

#include "platform.h"

#include "common/string_light.h"

#ifdef USE_GPS

// coordinates of anchor point
// TODO[uniks] make fc read and save anchor values from gps/pozyx module
#define ANCHOR_LAT 51.311644        // default anchor latitude 51.311644
#define ANCHOR_LON 9.473625         // default anchor longitude 9.473625
#define ANCHOR_ALTITUDE 166.0       // default anchor altitude 166m

// TODO[uniks] make fc use double values correctly
#define EARTH_R 6371001
#define ANCHOR_X 4905236.50         // default anchor x in m
#define ANCHOR_Y 818534.06          // default anchor y in m
#define ANCHOR_Z 3982515.75         // default anchor z in m

#define RAD_TO_DEGREE 57.2957795131 // 180/PI

#define DIGIT_TO_VAL(_x)    (_x - '0')

uint32_t GPS_coord_to_degrees(const char* coordinateString)
{
    const char *fieldSeparator, *remainingString;
    uint8_t degress = 0, minutes = 0;
    uint16_t fractionalMinutes = 0;
    uint8_t digitIndex;

    // scan for decimal point or end of field
    for (fieldSeparator = coordinateString; sl_isdigit((unsigned char)*fieldSeparator); fieldSeparator++) {
        if (fieldSeparator >= coordinateString + 15)
            return 0; // stop potential fail
    }
    remainingString = coordinateString;

    // convert degrees
    while ((fieldSeparator - remainingString) > 2) {
        if (degress)
            degress *= 10;
        degress += DIGIT_TO_VAL(*remainingString++);
    }
    // convert minutes
    while (fieldSeparator > remainingString) {
        if (minutes)
            minutes *= 10;
        minutes += DIGIT_TO_VAL(*remainingString++);
    }
    // convert fractional minutes
    // expect up to four digits, result is in
    // ten-thousandths of a minute
    if (*fieldSeparator == '.') {
        remainingString = fieldSeparator + 1;
        for (digitIndex = 0; digitIndex < 4; digitIndex++) {
            fractionalMinutes *= 10;
            if (sl_isdigit((unsigned char)*remainingString))
                fractionalMinutes += *remainingString++ - '0';
        }
    }
    return degress * 10000000UL + (minutes * 1000000UL + fractionalMinutes * 100UL) / 6;
}

void cartToSph(gpsDataPozyx_t* gps_Msg) {
	// TODO: implementation of orientation still needed for anchors

	double x = (double)ANCHOR_X + (gps_Msg->x_sign * gps_Msg->x/1000);
	double y = (double)ANCHOR_Y + (gps_Msg->y_sign * gps_Msg->y/1000);

	double altitude = gps_Msg->z_sign * gps_Msg->z/10;
	double z = (double)ANCHOR_Z + altitude;

//	double r  = sqrt(x*x+y*y+z*z); // FIXME[uniks] gives wrong answer, delete?
	double r  = (double)EARTH_R + (double)ANCHOR_ALTITUDE;
	double lat = acos(z/r) * (double)RAD_TO_DEGREE;
	double lon = atan2(y,x) * (double)RAD_TO_DEGREE;

	gps_Msg->latitude = lat*10000000;
	gps_Msg->longitude = lon*10000000;
	gps_Msg->altitude = altitude + (double)(ANCHOR_ALTITUDE*100.0);
}
#endif
