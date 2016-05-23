#pragma pack(1)
#include <cytypes.h>

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
    char  msg[256];
    uint8 sent;
    struct Message *next;
    struct Message *prev;
} Message;

typedef struct User {
    uint64      uniqueID;
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

void updateUsers(User **list, User *update);
User *findUser(User *list, uint64 id);
User *findUserAtPos(User *list, unsigned int pos);
#endif