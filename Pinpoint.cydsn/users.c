#include <cylib.h>
#include <stdlib.h>
#include "users.h"

/*
 * Returns the pointer to the user with the given ID if found,
 * or a pointer to null if not.
 */
User *findUser(User **list, uint64 id, int createNew) {
    User *temp, *cur = *list;
    
    while (cur) {
        if (cur->uniqueID == id)
           break;
        cur = cur->next;
    }
    
    if (!cur && createNew) {
        temp = calloc(sizeof(User), 1);
        temp->next = *list;
        *list = cur = temp;
        cur->uniqueID = id;
    }
    
    return cur;
}

/*
    Returns the pointer to the user at the given 
    position if found, or a pointer to null if not.
 */
User *findUserAtPos(User *list, unsigned int pos) {
    User *cur = list;
    
    while (cur && pos-- > 0) {
        cur = cur->next;
    }
    
    return cur;
}

void addMessage(User *user, char *msg, int sent) {
    Message *tmp = calloc(sizeof(Message), 1);
            
    // Add the message data
    strncpy(tmp->msg, msg, 256);
    tmp->msgLen = strlen(msg);
    tmp->sent = sent;
    
    // Add the message to the end of the list
    if (user->msgs) {
        // There are other messages
        tmp->next = user->msgs;
        tmp->prev = user->msgs->prev;
        user->msgs->prev->next = tmp;
        user->msgs->prev = tmp;
    }
    else {
        // This is the first message
        user->msgs = tmp->next = tmp->prev = tmp;
    }
    ++user->numMsgs;
}