#pragma pack(1)

#include <project.h>
#include "nmea.h"

typedef struct Self {
    char    name[20];
    uint32  id;
    GGA_Str gga;
    GSA_Str gsa;
    GSV_Str gsv;
    RMC_Str rmc;
    VTG_Str vtg;
} Self;

typedef struct XBEE_Header {
    char   name[20]; // Username of sender
    uint32 id;       // Unique serial id of sender
    double utc;      // Same format as GPS NMEA
    uint8  type;     // Message type
    uint32 dataLen;  // Length of the data only
} XBEE_Header;

void GPS_FurtherInit();