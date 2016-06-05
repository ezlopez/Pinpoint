#include "nmea.h"

/* Additional GPS setup:
 *    Temporarily kill all sentence output & clear buffer
 *    Set baudrate to 115200 (divider = 26 for 24 MHz clock)
 *    Set update rate to once per second
 *    Set NMEA output to be only RMC and GSA at every two seconds
 */
void GPS_FurtherInit() {
    CyDelay(1000);
    // Kill sentence output
    GPS_PutString("$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n");
    PC_PutString("Killing output\r\n");
    GPS_ClearRxBuffer();
    CyDelay(500); // Delay half a second to collect acknowledgement
    GPS_ClearRxBuffer();
    
    /*// Update the baud rate on the GPS chip and GPS UART module
    GPS_PutString("$PMTK251,115200*1F\r\n");
    GPS_CLK_SetDividerValue(26);
    PC_PutString("Changing baud rate\r\n");
    CyDelay(500); // Delay half a second to collect acknowledgement
    GPS_ClearRxBuffer();*/
    
    // Set GPS chip fix rate to 1 Hz
    GPS_PutString("$PMTK220,1000*1F\r\n");
    PC_PutString("Changing update rate\r\n");
    CyDelay(500); // Delay half a second to collect acknowledgement
    GPS_ClearRxBuffer();
    
    // Set GPS chip output to 0.5 Hz for RMC and GSA
    GPS_PutString("$PMTK314,0,2,0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0*2A\r\n");
    PC_PutString("Restarting output\r\n");
    CyDelay(500); // Delay half a second to collect acknowledgement
    GPS_ClearRxBuffer();
}

/* 
   This function parses the given sentence and puts the data in the
   struct pointed to by strStruct. Assumes strStruct is large enough
   to store the necessary data. To be safe, allocate as much memory as
   needed for the largest struct. Assumes the sentence is NULL-
   terminated. Returns the type of struct strStruct was used as. If
   the sentence is invalid, returns INVALID type and modifications to
   strStruct are undefined.
*/
nmea_type parseNMEA(char *sentence, void *strStruct) {
   nmea_type type;
   char *p = sentence + 7;
   int i, gsvMsgNum, satNum, maxSatNum, numSats = 0;

   if (!validateChecksum(sentence))
      return INVALID;

   if (!strncmp(sentence, "$GPGGA", 6)) {
      type = GGA;
      extract(type, p, "%lf", GGA_Str, strStruct, utc);
      extract(type, p, "%lf", GGA_Str, strStruct, lat);
      extract(type, p, "%c",  GGA_Str, strStruct, latDir);
      extract(type, p, "%lf", GGA_Str, strStruct, lon);
      extract(type, p, "%c",  GGA_Str, strStruct, lonDir);
      extract(type, p, "%hu", GGA_Str, strStruct, fix);
      extract(type, p, "%hu", GGA_Str, strStruct, numSats);
      extract(type, p, "%f",  GGA_Str, strStruct, hdop);
      extract(type, p, "%lf", GGA_Str, strStruct, alt);
      extract(type, p, "%c",  GGA_Str, strStruct, altUnits);
      extract(type, p, "%f",  GGA_Str, strStruct, geoidSep);
      extract(type, p, "%c",  GGA_Str, strStruct, geoidSepUnits);
      extract(type, p, "%f",  GGA_Str, strStruct, diffCorrAge);
    
      // Convert to signed Decimal Degree format
      GGA_Str *gga = (GGA_Str*)strStruct;
      gga->lat = gga->latDir == 'S' ? -1 * DDMtoDD(gga->lat) : DDMtoDD(gga->lat);
      gga->lon = gga->lonDir == 'W' ? -1 * DDMtoDD(gga->lon) : DDMtoDD(gga->lon);
   }
   else if (!strncmp(sentence, "$GPGSA", 6)) {
      type = GSA;
      extract(type, p, "%c",  GSA_Str, strStruct, mode1);
      extract(type, p, "%hu", GSA_Str, strStruct, mode2);
      for (i = 0; i < 12; i++)
         extract(type, p, "%hu", GSA_Str, strStruct, satsUsed[i]);
      extract(type, p, "%f",  GSA_Str, strStruct, pdop);
      extract(type, p, "%f",  GSA_Str, strStruct, hdop);
      extract(type, p, "%f",  GSA_Str, strStruct, vdop);
   }
   else if (!strncmp(sentence, "$GPGSV", 6)) {
      type = GSV;
      // Don't care about the number of messages. Just scanning the
      // message number.
      if (!sscanf(p + 2, "%d", &gsvMsgNum))
         type = INVALID;
      else {
         p += 4;
         --gsvMsgNum;
      }
      extract(type, p, "%hu", GSV_Str, strStruct, numSats);
      satNum = gsvMsgNum * 4;
      maxSatNum = numSats > satNum + 3 ? satNum + 3 : numSats;
      while (satNum <= maxSatNum) {
         extract(type, p, "%hu", GSV_Str, strStruct, satID[satNum]);
         extract(type, p, "%hu", GSV_Str, strStruct, elevation[satNum]);
         extract(type, p, "%hu", GSV_Str, strStruct, azimuth[satNum]);
         extract(type, p, "%hu", GSV_Str, strStruct, snr[satNum++]);
      }

      // Clear the memory of the later satellites. This means GSV
      // messages must be received in order.
      while (satNum < 12) {
         ((GSV_Str *)strStruct)->satID[satNum] = 0;
         ((GSV_Str *)strStruct)->elevation[satNum] = 0;
         ((GSV_Str *)strStruct)->azimuth[satNum] = 0;
         ((GSV_Str *)strStruct)->snr[satNum++] = 0;
      }
   }
   else if (!strncmp(sentence, "$GPRMC", 6)) {
      type = RMC;
      extract(type, p, "%lf", RMC_Str, strStruct, utc);
      extract(type, p, "%c",  RMC_Str, strStruct, status);
      extract(type, p, "%lf", RMC_Str, strStruct, lat);
      extract(type, p, "%c",  RMC_Str, strStruct, latDir);
      extract(type, p, "%lf", RMC_Str, strStruct, lon);
      extract(type, p, "%c",  RMC_Str, strStruct, lonDir);
      extract(type, p, "%f",  RMC_Str, strStruct, groundSpeed);
      extract(type, p, "%f",  RMC_Str, strStruct, groundCourse);
      extract(type, p, "%6u", RMC_Str, strStruct, date);
      extract(type, p, "%f",  RMC_Str, strStruct, magVar);
      extract(type, p, "%c",  RMC_Str, strStruct, magVarDir);
      if (type != INVALID) {
         ((RMC_Str *)strStruct)->mode = 0;
         extract(type, p, "%c",  RMC_Str, strStruct, mode);
         type = RMC; // Some modules don't implement mode, so just reset
      }
    
      // Convert to signed Decimal Degree format
      RMC_Str *rmc = (RMC_Str*)strStruct;
      rmc->lat = rmc->latDir == 'S' ? -1 * DDMtoDD(rmc->lat) : DDMtoDD(rmc->lat);
      rmc->lon = rmc->lonDir == 'W' ? -1 * DDMtoDD(rmc->lon) : DDMtoDD(rmc->lon);
   }
   else if (!strncmp(sentence, "$GPVTG", 6)) {
      type = VTG;
      extract(type, p, "%f", VTG_Str, strStruct, course[0]);
      extract(type, p, "%c", VTG_Str, strStruct, reference[0]);
      extract(type, p, "%f", VTG_Str, strStruct, course[1]);
      extract(type, p, "%c", VTG_Str, strStruct, reference[1]);
      extract(type, p, "%f", VTG_Str, strStruct, speed[0]);
      extract(type, p, "%c", VTG_Str, strStruct, speedUnits[0]);
      extract(type, p, "%f", VTG_Str, strStruct, speed[1]);
      extract(type, p, "%c", VTG_Str, strStruct, speedUnits[1]);
      if (type != INVALID) {
         ((VTG_Str *)strStruct)->mode = 0;
         extract(type, p, "%c", VTG_Str, strStruct, mode);
         type = VTG; // Some modules don't implement mode, so just reset
      }
   }
   else
      type = INVALID;

   return type;
}

/*
   Expects a standard NMEA string as input. Returns zero if the
   sentence is not valid or non-zero for a valid string. 
*/
int validateChecksum(char *sentence) {
   char *temp;
   uint16_t checksum, calculated = 0;
   int i;

   // Checking for the two defining characteristics of a NMEA string
   if (*sentence != '$' || !(temp = strchr(sentence, '*')))
      return 0;

   // Checking for a valid checksum
   if (!sscanf(temp, "*%02hx", &checksum))
      return 0;

   // Calculating the checksum from '$' to '*' (non-inclusive)
   for (i = 1; i < temp - sentence; i++)
      calculated ^= sentence[i];
    
   return calculated == checksum;
}

/*
    Converts the NMEA Degree and Decimal Minutes coordinate format to
    Decimal Degrees coordinate format.
*/
double DDMtoDD(double coord) {
    int degrees = coord / 100;
    double minutes = coord - (100 * degrees);
    
    return degrees + (minutes / 60.0);
}

/*
    
*/
float distance(float64 destLat, float64 destLon, float64 curLat, float64 curLon,
 float *latDist, float *lonDist) {
    *latDist = 3959 * M_PI / 180 * (destLat - curLat);
    *lonDist = 3959 * M_PI / 180 * (destLon - curLon)
             * cos(M_PI / 180 * (destLat + curLat) / 2.0);
    return sqrt(*latDist * *latDist + *lonDist * *lonDist);
}