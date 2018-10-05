unsigned long time;
double utc_time;
double lat;
double lng;
//char msg_utc[35];

static const double LAT_INCR = 0;
static const double LNG_INCR = 0.0001;
static const double FREQUENCY = 1;   //-> 1 HZ

static const char STR1[] = "$GPGGA,114449.345,";
static const char STR2[] = ",N,00";
static const char STR3[] = ",E,1,12,1.0,0.0,M,0.0,M,,*"; //67\r\n

//$GPRMC,083559.00,A,4717.11437,N,00833.91522,E,0.004,77.52,091202,,,A*57

static const char STR1_[] = "$GPRMC,,A,";
static const char STR2_[] = ",N,00";
static const char STR3_[] = ",E,,77.52,,,,A*";


//$GPRMC,hhmmss,status,latitude,N,longitude,E,spd,cog,ddmmyy,mv,mvE,mode*cs<CR><LF>


void setup() {
  Serial.begin(57600);
  lat = 5118.6950;
  lng = 928.4170;
}

void loop() {
  time = millis();

  // ######## WORKING STRING! ########
//  char gps[] = "$GPGGA,114449.345,5118.695,N,00928.417,E,1,12,1.0,0.0,M,0.0,M,,*67\r\n";
//  Serial.write(gps, strlen(gps));
  // ######## WORKING STRING! ########



  char msg_lat[30];
  char msg_lng[30];
//  strcpy(msg_lat, "$GPGGA,114449.345,");
  strcpy(msg_lat, STR1);
  dtostrf(lat, 2, 4, &msg_lat[strlen(msg_lat)]);
//  strcpy(msg_lng, ",N,00");
  strcpy(msg_lng, STR2);
  dtostrf(lng, 2, 4, &msg_lng[strlen(msg_lng)]);

//  char str[1+62+1+2];
//  sprintf(str, "%s%s%s",msg_lat, msg_lng, STR3 );

//  char msg[] = "$GPGGA,114449.345,5118.695,N,00928.417,E,1,12,1.0,0.0,M,0.0,M,,*";
//  byte crc = convertToCRC(str, strlen(str));
  
  
//  Serial.print(str);
//  Serial.println(crc, HEX);

  char str[] = "$GPRMC,083559.00,A,4717.11437,N,00833.91522,E,0.004,77.52,091202,,,A*";
  byte crc = convertToCRC(str, strlen(str));
//  Serial.print(str);
//  Serial.println(crc, HEX);

  Serial.println("$GPGGA,114449.345,5118.695,N,00928.417,E,1,12,1.0,0.0,M,0.0,M,,*67");
//  Serial.println("$GPGGA,092725.00,4717.11399,N,00833.91590,E,1,08,1.01,499.6,M,48.0,M,,*5B");
//  Serial.println("$GPRMC,083559.00,A,4717.11437,N,00833.91522,E,0.004,77.52,091202,,,A*57");
//  Serial.println("$GPVTG , 309.62,T, ,M,0.13,N,0.2,K,A*23");

  if(lat >= 5118.6950) {
    lat = lat - LAT_INCR;
  } else {
    lat = lat + LAT_INCR;
  }

  if(lng >= 00928.4210) {
    lng = lng - LNG_INCR;
  } else {
    lng = lng + LNG_INCR;
  }
  
  delay( (1/FREQUENCY * 1000) - (millis() - time) );
}



// ---------------------------------------------------------------
byte convertToCRC(char *buff, byte buff_len) {
  // NMEA CRC: XOR each byte with previous for all chars between '$' and '*'
  char c;
  byte i;
  byte start_with = 0;
  byte end_with = 0;
  byte crc = 0;

  for (i = 0; i < buff_len; i++) {
    c = buff[i];
    if(c == '$'){
      start_with = i;
    }
    if(c == '*'){
      end_with = i;
    }      
  }
  if (end_with > start_with){
    for (i = start_with+1; i < end_with; i++){ // XOR every character between '$' and '*'
      crc = crc ^ buff[i] ;  // compute CRC
    }
  }
  else { // else if error, print a msg (to both ports)
    Serial.println("CRC ERROR");
  }
  return crc;
  //based on code by Elimeléc López - July-19th-2013
}
