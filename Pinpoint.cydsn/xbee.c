#include <xbee.h>

/*
    Parses the message and enacts the appropriate action depending
    on the type of data in the message.
*/
void logXBdata(Self *me, void *data) {
    XBEE_Header *hdr = data;
    User *u;
    
    /* Find the user */
    //**********************************
    
    /* Copy the name */
    //**********************************
    
    if (hdr->type == POSITION) {
        XBEE_Position *pos = (XBEE_Position*)(hdr + 1);
        u->utc = pos->utc;
        u->pos = pos->pos;
    }
    else if (hdr->type == MESSAGE) {
        XBEE_Message *msg = (XBEE_Message*)(hdr + 1);
        
        // Write a newMessage function inside of user and replace inside display
        // Use that here too ***********************
    }
}

void broadcastPosition(Self *me) {
    XBEE_Header hdr;
    XBEE_Position pos;
    
    // Fill in the header
    hdr.destID = 0;
    memcpy(hdr.name, me->name, 20);
    hdr.srcID = me->id;
    hdr.type = POSITION;
    
    // Fill in the position
    pos.utc = me->rmc.utc;
    memcpy(&pos.pos, &me->rmc.lat, sizeof(Position));
    
    // Send the data
    XB_PutArray((uint8*)&hdr, sizeof(hdr));
    XB_PutArray((uint8*)&pos, sizeof(pos));
}

void sendMessage(Self *me, User *dest){
    XBEE_Header hdr;
    XBEE_Message msg;
    Message *tmp = calloc(sizeof(Message), 1);
            
    // Copy temp message data to message list
    strncpy(tmp->msg, dest->tempMsg.msg, dest->tempMsg.msgLen);
    tmp->msgLen = dest->tempMsg.msgLen;
    tmp->sent = 1;
    if (dest->msgs) {
        tmp->next = dest->msgs;
        tmp->prev = dest->msgs->prev;
        dest->msgs->prev->next = tmp;
        dest->msgs->prev = tmp;
    }
    else {
        tmp->next = tmp->prev = tmp;
        dest->msgs = tmp;
    }

    // Fill in the header
    hdr.destID = dest->uniqueID;
    memcpy(hdr.name, me->name, 20);
    hdr.srcID = me->id;
    hdr.type = POSITION;
    
    // Fill in the message
    memcpy(msg.msg, dest->tempMsg.msg, dest->tempMsg.msgLen);
    
    // Send the data
    XB_PutArray((uint8*)&hdr, sizeof(hdr));
    XB_PutArray((uint8*)&msg, sizeof(msg));
    
    // Clear out and update the count
    dest->tempMsg.msgLen = 0;
    ++dest->numMsgs;
}