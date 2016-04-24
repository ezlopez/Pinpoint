#pragma pack(1)

#include <project.h>
#include "nmea.h"
#include "users.h"

typedef struct Self {
    char    name[20];
    uint32  id;
    GGA_Str gga;
    GSA_Str gsa;
    GSV_Str gsv;
    RMC_Str rmc;
    VTG_Str vtg;
} Self;

typedef enum {POSITION, MESSAGE, PROBE_REQ, PROBE_RES} XB_Payload_Type;

typedef struct XBEE_Header {
    char   name[20]; // Username of sender
    uint32 id;       // Unique serial id of sender
    float64 utc;      // Same format as GPS NMEA
    XB_Payload_Type type;
    uint32 dataLen;  // Length of the data only
} XBEE_Header;

typedef struct XBEE_POSITION_MESSAGE {
    Position pos;
    float    pdop;
  //Position dest; // Maybe someday ***
} XBEE_POSITION_MESSAGE;

void GPS_FurtherInit();
void TFT_FurtherInit();
void logGPSdata();
void broadcastPosition();
void GPS_RXISR_ExitCallback();
void PC_RXISR_ExitCallback();
void XB_RXISR_ExitCallback();