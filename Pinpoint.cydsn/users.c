#include <cylib.h>
#include <stdlib.h>
#include "users.h"

/*
 * Updates the user list with the given information.
 * If the user does not exist, a new one is created and
 * appended to the end of the list.
 */
void updateUser(User *list, User *update) {
    User *user;
    
    if (!(user = findUser(list, update->uniqueID))) {
        // Go to the end and create a new user
        user = list;
        while (user->next)
           user = user->next;
        user->next = calloc(sizeof(User), 1);
        user = user->next;
    }
    
    // Update the info other than the next pointer
    cymemcpy(user, update, sizeof(User) - sizeof(User *));
}

/*
 * Returns the pointer to the user with the given ID if found,
 * or a pointer to null if not.
 */
User *findUser(User *list, uint64 id) {
    User *cur = list;
    
    while (cur) {
        if (cur->uniqueID == id)
           break;
        cur = cur->next;
    }
    
    return cur;
}