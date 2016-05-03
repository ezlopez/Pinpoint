#pragma pack(1)
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

typedef enum {POSITION, MESSAGE, PROBE_REQ, PROBE_RES} XB_Payload_Type;

typedef struct XBEE_Header {
    uint32 destID; // Either the unique ID of the intended recipient, or 0 for broadcast
    XB_Payload_Type type;
    uint32 dataLen;  // Length of the data only
} XBEE_Header;

void logGPSdata();
void broadcastPosition();
void GPS_RXISR_ExitCallback();
void PC_RXISR_ExitCallback();
void XB_RXISR_ExitCallback();
void logXBdata();