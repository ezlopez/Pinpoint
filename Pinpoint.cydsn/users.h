#pragma pack(1)
#include <cytypes.h>
#include <nmea.h>

#ifndef __USERS_H
#define __USERS_H
    
typedef struct Position {
    float64 lat;
    char   latDir; // N/S
    float64 lon;
    char   lonDir; // E/W
} Position;

typedef struct Message {
    uint8 msgLen;
    char  msg[255];
    uint8 sent;
    struct Message *next;
    struct Message *prev;
} Message;

typedef struct User {
    uint32      uniqueID;
    char        name[20];
    float64      utc;
    Position    pos;
    float       pdop;
    /* To be implemented later
    Position    dest;
    float64      alt;
    char        altUnits;
    */
    float       groundSpeed; // In knots
    float       groundCourse; // In degrees
    uint8       numMsgs;
    Message     *msgs;
    Message     tempMsg;
    struct User *next;
} User;

typedef struct Self {
    char    name[20];
    uint32  id;
    GGA_Str gga;
    GSA_Str gsa;
    GSV_Str gsv;
    RMC_Str rmc;
    VTG_Str vtg;
    User *users; // Users we have seen on the network
} Self;

User *findUser(User **list, uint64 id, int createNew);
User *findUserAtPos(User *list, unsigned int pos);
void addMessage(User *user, char *msg, int sent);
#endif