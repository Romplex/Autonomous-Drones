#include "gtest/gtest.h"
#include <stdint.h>
#include <math.h>


static const float ANCHOR_LAT = 51.311644;       // default anchor latitude 51.311644
static const float ANCHOR_LON = 9.473625;       // default anchor longitude 9.473625
static const float ANCHOR_ALTITUDE = 0;       // default anchor altitude 166m

// TODO[uniks] make fc use double values correctly
static const float EARTH_R = 6371001000;
static const float ANCHOR_X = 4905108868.795;         // default anchor x in m
static const float ANCHOR_Y = 818512691.684;        // default anchor y in m
static const float ANCHOR_Z = 3982411041.169;         // default anchor z in m

static const float RAD_TO_DEGREE = 180.0/M_PI; // 180/PI

typedef struct gpsDataPozyx_s {
    int32_t latitude;
    int32_t longitude;

    int32_t x;
    int32_t y;
    int32_t z;

    int x_sign;
    int y_sign;
    int z_sign;

    int32_t altitude;
} gpsDataPozyx_t;

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

void static cartToSph(gpsDataPozyx_t* gps_Msg) {
	// TODO: implementation of orientation still needed for anchors

	float x = ANCHOR_X + (gps_Msg->x_sign * gps_Msg->x);
	float y = ANCHOR_Y + (gps_Msg->y_sign * gps_Msg->y);
	float z = ANCHOR_Z + gps_Msg->z;

	EXPECT_EQ(x, ANCHOR_X) << "anchor x test";
	EXPECT_EQ(y, ANCHOR_Y) << "anchor y test";

//	double r  = sqrt(x*x+y*y+z*z); // FIXME[uniks] gives wrong answer, delete?
	float r  = EARTH_R + ANCHOR_ALTITUDE;
	float lat = acos(z/r) * RAD_TO_DEGREE;
	float lon = atan2(y,x) * RAD_TO_DEGREE;

	EXPECT_FLOAT_EQ(lat, ANCHOR_LAT)<< "anchor latitude test";
	//EXPECT_FLOAT_EQ(acos(z/r), 1) << "test acos";

	gps_Msg->latitude = lat*1000000;

	EXPECT_EQ(gps_Msg->latitude, 51311644) << "anchor latitude test";

	gps_Msg->longitude = lon*1000000;
	gps_Msg->altitude = r - EARTH_R;
}

static void test() {
    uint32_t value = grab_fields("51311644", 0);
//    EXPECT_EQ(value, 51311644) << "###### Grab Field Test";


}
static void test2() {
	gpsDataPozyx_t d;
	d.x = 0;
	d.y = 0;
	d.z = 0;

	d.z_sign = 1;
	d.x_sign = 1;
	d.y_sign = 1;

	cartToSph(&d);


	EXPECT_FLOAT_EQ(d.latitude, ANCHOR_LAT*1000000) << "latitude conversion test";
	//EXPECT_EQ(d.longitude, ANCHOR_LON) << "longitude conversion test";

}

TEST(SerialTest, ConversionCartToSphTest) {
	test2();
}

TEST(SerialTest, GrabFieldTest) {
	test();
}

