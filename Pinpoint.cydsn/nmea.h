#pragma pack(1)

#include <project.h>
#include <cytypes.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef __NMEA_H
#define __NMEA_H

typedef enum {GGA, GSA, GSV, RMC, VTG, INVALID} nmea_type;

#define extract(_type, _ptr, _format, _enumType, _struct, _pos) {\
   char *_temp;\
   if (_type == INVALID || *_ptr == '\0' || *_ptr == '\r')\
      _type = INVALID;\
   else if (*_ptr == ',') {\
      ++_ptr;\
      ((struct _enumType*)_struct)->_pos = 0;\
   }\
   else {\
      if (!strcmp("%f", _format) || !strcmp("%lf", _format)) {\
         /* PSOC doesn't handle foating point scanning very well */\
         ((struct _enumType*)_struct)->_pos = atof(_ptr);\
      }\
      else {\
         if (!sscanf(_ptr, _format, &((struct _enumType*)_struct)->_pos))\
            type = INVALID;\
      }\
      if ((_temp = strchr(_ptr, ',')))\
         _ptr = _temp + 1;\
      else if((_temp = strchr(_ptr, '\0')))\
         _ptr = _temp;\
      else\
         _type = INVALID;\
   }\
}

typedef struct GGA_Str {
   float64 utc;
   float64 lat;
   char   latDir; // N/S
   float64 lon;
   char   lonDir; // E/W
   uint16_t fix; // 1 = No fix, 2 = GPS fix, 3 = DGPS fix
   uint16_t numSats;
   float  hdop;
   float64 alt;
   char   altUnits;
   float  geoidSep;
   char   geoidSepUnits;
   float  diffCorrAge;
} GGA_Str;

typedef struct GSA_Str {
   char   mode1; // M = manual, A = 2D automatic
   uint16_t mode2; // 1 = no fix, 2 = 2D (<4 sats), 3 = 3D (>=4 sats)
   uint16_t satsUsed[12]; // The SV on each channel
   float  pdop;
   float  hdop;
   float  vdop;
} GSA_Str;

// The arrays are filled at respective satellite channel numbers.
// E.g. the information for the first satellite from a GSV sentence
// will be found at position zero in the ID, elevation, azimuth, and
// SNR arrays. Adjust the sizes of the arrays for your GPS chip's needs.
typedef struct GSV_Str {
   uint16_t  numSats;
   uint16_t  satID[12];
   uint16_t  elevation[12]; // In degrees
   uint16_t  azimuth[12];
   uint16_t  snr[12]; // 0 to 99, null when not tracked
} GSV_Str;

typedef struct RMC_Str {
   float64  utc;
   char    status; // A = valid, V = not valid
   float64  lat;
   char    latDir; // N/S
   float64  lon;
   char    lonDir; // E/W
   float   groundSpeed; // In knots
   float   groundCourse; // In degrees
   uint32_t date;
   float   magVar; // Degrees
   char    magVarDir; // E/W
   char    mode; // A = autonomous, D = differential, E = estimated
} RMC_Str;

typedef struct VTG_Str {
   float course[2];
   char  reference[2]; // T = true, M = magnetic
   float speed[2];
   char  speedUnits[2]; // N = knots, K = kph
   char  mode; // A = autonomous, D = differential, E = estimated
} VTG_Str;

void GPS_FurtherInit();
nmea_type parseNMEA(char *sentence, void *strStruct);
int validateChecksum(char *sentence);
double DDMtoDD(double coord);
#endif