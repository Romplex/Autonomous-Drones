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

#define DIGIT_TO_VAL(_x)    (_x - '0')

// coordinates of anchor point
// TODO[uniks] make fc read and save anchor values from gps/pozyx module
static const float ANCHOR_LAT = 51.311644;        // default anchor latitude 51.311644
static const float ANCHOR_LON = 9.473625;         // default anchor longitude 9.473625
static const float ANCHOR_ALTITUDE = 0;       // default anchor altitude 166m

// TODO[uniks] make fc use float values correctly
static const float EARTH_R  = 6371001000.0;          // earth radius in mm
static const float ANCHOR_X = 4905108868.795;         // default anchor x in mm
static const float ANCHOR_Y = 818512691.684;          // default anchor y in mm
static const float ANCHOR_Z = 3982411041.169;         // default anchor z in mm

/*static const float ANCHOR_LAT = 51.311644;        // default anchor latitude 51.311644
static const float ANCHOR_LON = 9.473625;         // default anchor longitude 9.473625
static const float ANCHOR_ALTITUDE = 166.0;       // default anchor altitude 166m

static const float EARTH_R  = 6371001.0;          // earth radius in m
static const float ANCHOR_X = 4905108.87;         // default anchor x in m
static const float ANCHOR_Y = 818512.69;          // default anchor y in m
static const float ANCHOR_Z = 3982411.04;         // default anchor z in m*/

static const float RAD_TO_DEGREE = 180.0/M_PI; // 180/PI

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

	float x = ANCHOR_X + (gps_Msg->x_sign * gps_Msg->x);
	float y = ANCHOR_Y + (gps_Msg->y_sign * gps_Msg->y);

	float altitude = gps_Msg->z_sign * gps_Msg->z;
	float z = ANCHOR_Z + altitude;

//	float r  = sqrt(x*x+y*y+z*z); // FIXME[uniks] gives wrong answer, delete?
	float r  = EARTH_R + ANCHOR_ALTITUDE;
	float lat = (float)acos(z/r) * RAD_TO_DEGREE;
	float lon = (float)atan2(y,x) * RAD_TO_DEGREE;

	gps_Msg->latitude = lat*1000000;
	gps_Msg->longitude = lon*1000000;
	gps_Msg->altitude = (altitude + ANCHOR_ALTITUDE)/1000.0;
}
#endif
