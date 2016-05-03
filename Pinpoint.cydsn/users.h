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
    struct User *next;
} User;

void updateUsers(User **list, User *update);
User *findUser(User *list, uint64 id);

#endif