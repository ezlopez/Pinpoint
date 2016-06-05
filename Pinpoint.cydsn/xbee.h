#pragma pack(1)

#ifndef __XBEE_H__
#define __XBEE_H__

#include <users.h>
#include <Adafruit_RA8875.h>
#include "display.h"
    
typedef enum {POSITION, MESSAGE, PROBE_REQ} XB_Payload_Type;

typedef struct XBEE_Header {
    uint32 destID; // uID of the dest, or 0 for broadcast
    uint32 srcID;
    char  name[20];
    XB_Payload_Type type;
} XBEE_Header;

typedef struct XBEE_Position {
    float64  utc;
    Position pos;
} XBEE_Position;

typedef struct XBEE_Message {
    char  msg[255];
} XBEE_Message;

static const uint XBEE_STR_SIZE[] = {sizeof(XBEE_Position), sizeof(XBEE_Message), 0};

void broadcastPosition(Self *me);
void logXBdata(Self *me, void *data);
void sendMessage(Self *me, User *dest);
#endif