#include "nmea.h"

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
   int i, gsvMsgNum, numSats, satNum;

   if (!validateChecksum(sentence))
      return INVALID;

   if (!strncmp(sentence, "$GPGGA", 6)) {
      type = GGA;
      extract(type, p, "%6.5lf", GGA_Str, strStruct, utc);
      extract(type, p, "%4.4lf", GGA_Str, strStruct, lat);
      extract(type, p, "%c",  GGA_Str, strStruct, latDir);
      extract(type, p, "%5.4lf", GGA_Str, strStruct, lon);
      extract(type, p, "%c",  GGA_Str, strStruct, lonDir);
      extract(type, p, "%hu", GGA_Str, strStruct, fix);
      extract(type, p, "%hu", GGA_Str, strStruct, numSats);
      extract(type, p, "%3.3f",  GGA_Str, strStruct, hdop);
      extract(type, p, "%6.5lf", GGA_Str, strStruct, alt);
      extract(type, p, "%c",  GGA_Str, strStruct, altUnits);
      extract(type, p, "%3.3f",  GGA_Str, strStruct, geoidSep);
      extract(type, p, "%c",  GGA_Str, strStruct, geoidSepUnits);
      extract(type, p, "%3.3f",  GGA_Str, strStruct, diffCorrAge);
   }
   else if (!strncmp(sentence, "$GPGSA", 6)) {
      type = GSA;
      extract(type, p, "%c",  GSA_Str, strStruct, mode1);
      extract(type, p, "%hu", GSA_Str, strStruct, mode2);
      for (i = 0; i < 12; i++)
         extract(type, p, "%hu", GSA_Str, strStruct, satsUsed[i]);
      extract(type, p, "%3.3f",  GSA_Str, strStruct, pdop);
      extract(type, p, "%3.3f",  GSA_Str, strStruct, hdop);
      extract(type, p, "%3.3f",  GSA_Str, strStruct, vdop);
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
      numSats = ((GSV_Str *)strStruct)->numSats - satNum;
      if (numSats > 4)
         numSats = 4;
      while (numSats--) {
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
      extract(type, p, "%6.3lf", RMC_Str, strStruct, utc);
      extract(type, p, "%c",  RMC_Str, strStruct, status);
      extract(type, p, "%4.4f", RMC_Str, strStruct, lat);
      extract(type, p, "%c",  RMC_Str, strStruct, latDir);
      extract(type, p, "%5.4lf", RMC_Str, strStruct, lon);
      extract(type, p, "%c",  RMC_Str, strStruct, lonDir);
      extract(type, p, "%3.3f",  RMC_Str, strStruct, groundSpeed);
      extract(type, p, "%3.3f",  RMC_Str, strStruct, groundCourse);
      extract(type, p, "%6u", RMC_Str, strStruct, date);
      extract(type, p, "%3.3f",  RMC_Str, strStruct, magVar);
      extract(type, p, "%c",  RMC_Str, strStruct, magVarDir);
      if (type != INVALID) {
         ((RMC_Str *)strStruct)->mode = 0;
         extract(type, p, "%c",  RMC_Str, strStruct, mode);
         type = RMC; // Some modules don't implement mode, so just reset
      }
   }
   else if (!strncmp(sentence, "$GPVTG", 6)) {
      type = VTG;
      extract(type, p, "%3.3f", VTG_Str, strStruct, course[0]);
      extract(type, p, "%c", VTG_Str, strStruct, reference[0]);
      extract(type, p, "%3.3f", VTG_Str, strStruct, course[1]);
      extract(type, p, "%c", VTG_Str, strStruct, reference[1]);
      extract(type, p, "%3.3f", VTG_Str, strStruct, speed[0]);
      extract(type, p, "%c", VTG_Str, strStruct, speedUnits[0]);
      extract(type, p, "%3.3f", VTG_Str, strStruct, speed[1]);
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
   Returns the size of the largest NMEA sentence struct as returned by
   the sizeof operator.
*/
size_t sizeofLargest() {
   size_t max = sizeof(struct GGA_Str);

   max = sizeof(struct GSA_Str) > max ? sizeof(struct GSA_Str) : max;
   max = sizeof(struct GSV_Str) > max ? sizeof(struct GSV_Str) : max;
   max = sizeof(struct RMC_Str) > max ? sizeof(struct RMC_Str) : max;
   max = sizeof(struct VTG_Str) > max ? sizeof(struct VTG_Str) : max;

   return max;
}
