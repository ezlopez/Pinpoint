#include <xbee.h>

/*
    Parses the message and enacts the appropriate action depending
    on the type of data in the message.
*/
void logXBdata(Self *me, void *data) {
    XBEE_Header *hdr = data;
    User *u = findUser(&me->users, hdr->srcID, 1);
    
    /* Copy the name */
    memcpy(u->name, hdr->name, 20);
    
    if (hdr->type == POSITION) {
        XBEE_Position *pos = (XBEE_Position*)(hdr + 1);
        // Update the position
        u->utc = pos->utc;
        u->pos = pos->pos;
        if (curMenu == MENU_HOME)
           Disp_Refresh_Map();
    }
    else if (hdr->type == MESSAGE) {
        XBEE_Message *msg = (XBEE_Message*)(hdr + 1);
        // Add the message to the user's list
        addMessage(u, msg->msg, 0);
        Adafruit_RA8875_textMode();
        Adafruit_RA8875_textEnlarge(2);
        Adafruit_RA8875_textSetCursor(170, 100);
        Adafruit_RA8875_textColor(RA8875_RED, RA8875_BLACK);
        Adafruit_RA8875_textWrite("New Message", 11);
    }
}

void broadcastPosition(Self *me) {
    XBEE_Header hdr;
    XBEE_Position pos;
    
    // Clear out the timer interrupt
    Broadcast_Timer_ReadStatusRegister();
    
    // Fill in the header
    hdr.destID = 0;
    memcpy(hdr.name, me->name, 20);
    hdr.srcID = me->id;
    hdr.type = POSITION;
    
    // Fill in the position
    pos.utc = me->rmc.utc;
    memcpy(&pos.pos, &me->rmc.lat, sizeof(Position));
    
    // Send the data
    XB_PutArray((uint8*)&hdr, sizeof(XBEE_Header));
    XB_PutArray((uint8*)&pos, sizeof(XBEE_Position));
    XB_PutArray((uint8*)"***", 3);
}

void sendMessage(Self *me, User *dest){
    XBEE_Header hdr;
    XBEE_Message msg;
    
    // Add the message to the user's list
    addMessage(dest, dest->tempMsg.msg, 1);

    // Fill in the header
    hdr.destID = dest->uniqueID;
    memcpy(hdr.name, me->name, 20);
    hdr.srcID = me->id;
    hdr.type = MESSAGE;
    
    // Fill in the message
    memcpy(msg.msg, dest->tempMsg.msg, 250);
    
    // Send the data
    XB_PutArray((uint8*)&hdr, sizeof(XBEE_Header));
    XB_PutArray((uint8*)&msg, sizeof(XBEE_Message));
    XB_PutArray((uint8*)"***", 3);
    
    // Clear out the temp message
    dest->tempMsg.msgLen = 0;
    dest->tempMsg.msg[0] = 0;
}