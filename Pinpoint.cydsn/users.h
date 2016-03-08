#pragma pack(1)
#include <cytypes.h>

typedef struct Position {
    double lat;
    char   latDir; // N/S
    double lon;
    char   lonDir; // E/W
} Position;

typedef struct User {
    uint64      uniqueID;
    char        name[15];
    double      utc;
    Position    pos;
    Position    dest;
    float       pdop;
    double      alt;
    char        altUnits;
    float       groundSpeed; // In knots
    float       groundCourse; // In degrees
    struct User *next;
} User;

void updateUser(User *list, User *update);
User *findUser(User *list, uint64 id);