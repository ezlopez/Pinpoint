#include "display.h"
#include <project.h>

/*
    Turns all functions of the display on, runs through the touch-screen
    callibration with the user, and draws the home screen.
*/
void Disp_FurtherInit(Self *me) {
    int i;
    
    if (!Adafruit_RA8875_begin()) {
        PC_PutString("Failed to init TFT.\r\n");
        return;
    }
    PC_PutString("TFT init succeeded.\r\n");
    
    Adafruit_RA8875_displayOn(1);
    Adafruit_RA8875_GPIOX(1); // Enable TFT - display enable tied to GPIOX
    Adafruit_RA8875_PWM1config(1, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
    Adafruit_RA8875_PWM1out(255);
    Adafruit_RA8875_setOrientation(1);
    
    // 3-Point Touch screen callibration
    // https://www.maximintegrated.com/en/app-notes/index.mvp/id/5296
    uint16 z[9] = {0,0,1,
                   0,0,1,
                   0,0,1};
    int testX[3] = {700, 135, 452};
    int testY[3] = {400, 210, 108};
    
    Adafruit_RA8875_graphicsMode();
    Adafruit_RA8875_touchEnable(1);
    for (i = 0; i < 3; i++) {
        Adafruit_RA8875_fillScreen(RA8875_BLACK);
        Adafruit_RA8875_fillCircle(testX[i], testY[i], 5, RA8875_WHITE);
        // Clear out old touches
        if (Adafruit_RA8875_touched())
            Adafruit_RA8875_touchRead(NULL, NULL);
        while(!Adafruit_RA8875_touched());
        Adafruit_RA8875_touchRead(z + 3 * i, 1 + z + 3 * i);
        CyDelay(300);
    }
    if (Adafruit_RA8875_touched())
        Adafruit_RA8875_touchRead(NULL, NULL);
            
    double z_inv[9] = {z[4]*z[8] - z[5]*z[7], z[2]*z[7] - z[1]*z[8], z[1]*z[5] - z[2]*z[4],
                       z[5]*z[6] - z[3]*z[8], z[0]*z[8] - z[2]*z[6], z[2]*z[3] - z[0]*z[5],
                       z[3]*z[7] - z[4]*z[6], z[1]*z[6] - z[0]*z[7], z[0]*z[4] - z[1]*z[3]};
    
    double det_z = z[0] * z_inv[0] + z[1] * z_inv[3] + z[2] * z_inv[6];
    
    for (i = 0; i < 9; i++)
       z_inv[i] /= det_z;
    
    tsCalCoeff[0] = z_inv[0]*testX[0] + z_inv[1]*testX[1] + z_inv[2]*testX[2];
    tsCalCoeff[1] = z_inv[3]*testX[0] + z_inv[4]*testX[1] + z_inv[5]*testX[2];
    tsCalCoeff[2] = z_inv[6]*testX[0] + z_inv[7]*testX[1] + z_inv[8]*testX[2];
    tsCalCoeff[3] = z_inv[0]*testY[0] + z_inv[1]*testY[1] + z_inv[2]*testY[2];
    tsCalCoeff[4] = z_inv[3]*testY[0] + z_inv[4]*testY[1] + z_inv[5]*testY[2];
    tsCalCoeff[5] = z_inv[6]*testY[0] + z_inv[7]*testY[1] + z_inv[8]*testY[2];
    
    // Finally show the home screen
    myself = me;
    goToMenu(MENU_HOME, NULL);
    
    PC_PutString("Finished TFT init\r\n");
}

/*
    Returns 1 and updates the values in x and y to be the pixel
    coordinates of the last touch. Else, returns 0;
*/
int Disp_Get_Touch(uint16 *x, uint16 *y) {
    if (Adafruit_RA8875_touched()) {
        uint16 tempX, tempY;
        Adafruit_RA8875_touchRead(&tempX, &tempY);
        *x = tempX * tsCalCoeff[0] + tempY * tsCalCoeff[1] + tsCalCoeff[2];
        *y = tempX * tsCalCoeff[3] + tempY * tsCalCoeff[4] + tsCalCoeff[5];
        return 1;
    }
    
    return 0;
}

/*
    Performs the action requested by the user's touch, if any.
*/
void Disp_touchResponse(int x, int y) {
    int listInd;
    User *u;
    
    
    // Back is on all menus but home
    if (curMenu != MENU_HOME && (BUTTON_HIT(x, y, 750, 0))) {
        PC_PutString("Back button\r\n");
        if (curMenu == MENU_COMPOSE)
            goToMenu(MENU_CONVERSATION, curConvo);
        else
            goToMenu(previous[curMenu], NULL);
            
        return;
    }
    
    switch(curMenu) {
        case MENU_HOME:
            if (BUTTON_HIT(x, y, 0, 0)) {
                PC_PutString("Settings button\r\n");
                goToMenu(MENU_SETTINGS, NULL);
            }
            else if (BUTTON_HIT(x, y, 0, 165)) {
                PC_PutString("Messages button\r\n");
                goToMenu(MENU_MESSAGES, NULL);
            }
            else if (BUTTON_HIT(x, y, 0, 330)) {
                PC_PutString("Info button\r\n");
                goToMenu(MENU_INFO, NULL);
            }
            break;
        case MENU_SETTINGS:
            break;
        case MENU_MESSAGES:
            listInd = (LIST_POS(x, 100, 50, 13));
            if (listInd >= 0 && (u = findUserAtPos(myself->users, listInd))) {
                goToMenu(MENU_CONVERSATION, u);
            }
            break;
        case MENU_INFO:
            listInd = (LIST_POS(x, 100, 50, 13));
            if (listInd >= 0) {
                if (listInd == 0) {
                    goToMenu(MENU_INFO_DETAILS, myself);
                }
                else if ((u = findUserAtPos(myself->users, --listInd))){
                    goToMenu(MENU_INFO_DETAILS, u);
                }
            }
            break;
        case MENU_NAME_EDIT:
            break;
        case MENU_CONVERSATION:
            if (BUTTON_HIT(x, y, 750, 330)) {
                goToMenu(MENU_COMPOSE, curConvo);
            }
            break;
        case MENU_COMPOSE:
            updateMessage(x, y);
            break;
        case MENU_INFO_DETAILS:
            if (BUTTON_HIT(x, y, 300, 0)) {
                PC_PutString("Messages button\r\n");
                if (curDetails == myself)
                    goToMenu(MENU_MESSAGES, NULL);
                else
                    goToMenu(MENU_CONVERSATION, curDetails);
            }
            break;
        default:
            PC_PutString("Invalid menu\r\n");
            break;
    }
}

/*
    Updates the map to show any changes in User positions.
    If there are no users, it prints an empty map with default
    max distance of 2.0 miles.
*/
void Disp_Refresh_Map() {
     // If you have more than 30 users, fix this
    double latDist[30], lonDist[30], totDist;
    int i, numInvalid = 0;
    double maxDist = 6.0/4; // So that the default is 2.0
    double transX, transY;
    char text[100];
    User *u = myself->users;
    Position *my_pos = (Position*)&(myself->rmc.lat);
    
    /* Switch to graphics mode and print the map */
    Adafruit_RA8875_graphicsMode();
    
    // Clear the map area
    Adafruit_RA8875_rectHelper(200, 0, 760, 479, RA8875_BLACK, 1);
    
    // Redraw the bullseye
    Adafruit_RA8875_drawCircle(500, 240, 239, RA8875_WHITE);
    Adafruit_RA8875_drawCircle(500, 240, 160, RA8875_WHITE);
    Adafruit_RA8875_drawCircle(500, 240, 80, RA8875_WHITE);
    Adafruit_RA8875_fillCircle(500, 240, 7, RA8875_WHITE);
    
    // Find all the distances
    for (i = 0; u; i++, u = u->next) {
        // Only calculate if ours and the user's positions are valid
        if (u->pos.latDir && my_pos->latDir) {
            latDist[i] = 3959 * M_PI / 180 * (my_pos->lat - u->pos.lat);
            lonDist[i] = 3959 * M_PI / 180 * (my_pos->lon - u->pos.lon)
             * cos(M_PI / 180 * (my_pos->lat + u->pos.lat) / 2.0);
            totDist = sqrt(latDist[i] * latDist[i] + lonDist[i] * lonDist[i]);
            if (totDist > maxDist)
                maxDist = totDist;
        }
    }
    
    maxDist *= 4.0 / 3;
    
    // Print the ring labels
    Adafruit_RA8875_textMode();
    Adafruit_RA8875_textEnlarge(0);
    Adafruit_RA8875_textColor(RA8875_WHITE, RA8875_BLACK);
    
    Adafruit_RA8875_textSetCursor(730, 210);
    sprintf(text, "%0.1f mi", maxDist);
    Adafruit_RA8875_textWrite(text, strlen(text));
    
    Adafruit_RA8875_textSetCursor(650, 210);
    sprintf(text, "%0.1f mi", maxDist * 2 / 3);
    Adafruit_RA8875_textWrite(text, strlen(text));
    
    Adafruit_RA8875_textSetCursor(570, 210);
    sprintf(text, "%0.1f mi", maxDist / 3);
    Adafruit_RA8875_textWrite(text, strlen(text));
    
    if (!myself->users) {
        Adafruit_RA8875_textEnlarge(0);
        Adafruit_RA8875_textColor(RA8875_RED, RA8875_BLACK);
        Adafruit_RA8875_textSetCursor(240, 160);
        Adafruit_RA8875_textWrite("Searching for users...", 22);
    }
    else if (!my_pos->latDir) {
        Adafruit_RA8875_textEnlarge(0);
        Adafruit_RA8875_textColor(RA8875_RED, RA8875_BLACK);
        Adafruit_RA8875_textSetCursor(240, 160);
        Adafruit_RA8875_textWrite("Waiting for location...", 23);
    }
    
    Adafruit_RA8875_graphicsMode();
    // Paint all the users
    for (u = myself->users, i = 0; u; i++, u = u->next) {
        if (u->pos.latDir != 0 && my_pos->latDir) {
            transX = 240 * latDist[i] / maxDist;
            transY = 240 * lonDist[i] / maxDist;
            Adafruit_RA8875_fillCircle(500 + transX, 240 - transY, 7, u->uniqueID & RA8875_WHITE);
        }
        else
            ++numInvalid;
    }
    
    if (numInvalid) {
        Adafruit_RA8875_textMode();
        Adafruit_RA8875_textSetCursor(220, 150);
        Adafruit_RA8875_textEnlarge(0);
        Adafruit_RA8875_textColor(RA8875_RED, RA8875_BLACK);
        sprintf(text, "%d unknown positions", numInvalid);
        Adafruit_RA8875_textWrite(text, strlen(text));
        
    }
}

/*
    Prints the time on the bottom-right corner of the screen.
    Only prints when the minute changes unless <force> is set
    to 1;
*/
void Disp_Update_Time(int force) {
    int utc = myself->rmc.utc;
    static int prevMin = -1;
    char text[13];
    int hours = utc / 10000;
    int minutes = (utc % 10000) / 100;
    char *meridiem;
    char *format;
    
    if ((minutes != prevMin && curMenu == MENU_HOME) || force) {
        // Change to PDT, doesn't work during daylight savings
        hours = (hours + 17) % 24;
        
        // Convert to 12-hour format
        if (hours >= 12)
            meridiem = "pm";
        else
            meridiem = "am";
        hours %= 12;
        if (hours == 0)
            hours = 12;
        
        // Making sure to keep alignment
        if (hours < 10)
            format = " %d:%02d %s";
        else
            format = "%d:%02d %s";
    
        Adafruit_RA8875_textMode();
        Adafruit_RA8875_textEnlarge(1);
        Adafruit_RA8875_textSetCursor(760, 340);
        Adafruit_RA8875_textColor(RA8875_YELLOW, RA8875_BLACK);
        sprintf(text, format, hours, minutes, meridiem);
        Adafruit_RA8875_textWrite(text, strlen(text));
        prevMin = minutes;
    }
}

/*
    Updates the compose menu after a touch response.
    <x> and <y> are the screen coordinates for the touch.
*/
void updateMessage(int x, int y) {
    Message *m = &curConvo->tempMsg;
    static int cap = 0;
    int key = -1;
    const char keys[] = {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
                         'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
                          0,  'z', 'x', 'c', 'v', 'b', 'n', 'm',  0 , ' '};
    
    // Find which key was pressed
    if (x >= 588 && x <= 636) {
        // First row
        key = y / 48;
    }
    else if (x >= 642 && x <= 690 && y >= 24 && y <= 432) {
        // Second row
        key = (int)((y - 24) / 48) + 10;
    }
    else if (x >= 696 && x <= 744 && y >= 24 && y <= 432) {
        // Third row
        key = (int)((y - 24) / 48) + 19;
    }
    else if (x >= 750) {
        // Bottom row
        if (y >= 160 && y <= 320)
            key = 28;
        else if (y >= 330) {
            Message *tmp = calloc(sizeof(Message), 1);
            // Send the message
Adafruit_RA8875_textMode();
Adafruit_RA8875_textEnlarge(2);
Adafruit_RA8875_textColor(RA8875_WHITE, RA8875_RED);
Adafruit_RA8875_textSetCursor(0, 0);
Adafruit_RA8875_textWrite("Implement sending", 17);
CyDelay(1000);
            // Copy message data to message list
            strncpy(tmp->msg, m->msg, m->msgLen);
            tmp->msgLen = m->msgLen;
            tmp->sent = 1;
            if (curConvo->msgs) {
                tmp->next = curConvo->msgs;
                tmp->prev = curConvo->msgs->prev;
                curConvo->msgs->prev->next = tmp;
                curConvo->msgs->prev = tmp;
            }
            else {
                tmp->next = tmp->prev = tmp;
                curConvo->msgs = tmp;
            }
            
            // Clear the temp message and go back to the convo
            m->msgLen = 0;
            goToMenu(MENU_CONVERSATION, curConvo);
        }
    }
    
    if (key == 19) {
        cap = !cap;
    }
    else if (key >= 0) {
        int lineNum = (m->msgLen - 1) / 20;
        
        if (key == 27 && m->msgLen) {
            --m->msgLen;
            // Clear the last line
            Adafruit_RA8875_graphicsMode();
            Adafruit_RA8875_fillRect(lineNum * 48, 0, 48, 480, RA8875_BLACK);
        }
        else if (key != 27 && m->msgLen < 240) {
            // Update the message
            m->msg[m->msgLen++] = keys[key] - (cap && key != 28) * 32;
        }
            
        // Write the last line
        if (m->msgLen) {
            int mod = m->msgLen % 20;
            Adafruit_RA8875_textMode();
            Adafruit_RA8875_textEnlarge(2);
            Adafruit_RA8875_textTransparent(RA8875_WHITE);
            Adafruit_RA8875_textSetCursor(lineNum * 48, 0);
            Adafruit_RA8875_textWrite(m->msg + 20 * lineNum, mod ? mod : 20);
        }
    }
}

/*
    Draws the Home screen
*/
void drawHome() {
    /* Print the buttons */
    Adafruit_RA8875_graphicsMode();
    Adafruit_RA8875_fillScreen(RA8875_BLACK);
    Adafruit_RA8875_fillRoundRect(0,   0, 50, 150, 5, RA8875_BLUE);
    Adafruit_RA8875_fillRoundRect(0, 165, 50, 150, 5, RA8875_BLUE);
    Adafruit_RA8875_fillRoundRect(0, 330, 50, 150, 5, RA8875_BLUE);

    /* Print the title */  
    Adafruit_RA8875_textMode();
    Adafruit_RA8875_textSetCursor(70, 120);
    Adafruit_RA8875_textTransparent(RA8875_CYAN);
    Adafruit_RA8875_textEnlarge(2);
    Adafruit_RA8875_textWrite("Pinpoint!", 9);
    
    /* Print the name */ 
    Adafruit_RA8875_textEnlarge(1);
    Adafruit_RA8875_textSetCursor(760, 10);
    Adafruit_RA8875_textTransparent(RA8875_YELLOW);
    Adafruit_RA8875_textWrite(myself->name, strlen(myself->name));
    
    /* Print the button labels */
    Adafruit_RA8875_textTransparent(RA8875_WHITE);
    Adafruit_RA8875_textSetCursor(7, 12);
    Adafruit_RA8875_textWrite("Settings", 8);
    Adafruit_RA8875_textSetCursor(7, 370);
    Adafruit_RA8875_textWrite("Info", 4);
    Adafruit_RA8875_textSetCursor(7, 177);
    Adafruit_RA8875_textWrite("Messages", 8);
    
    /* Print the rest */
    Disp_Refresh_Map();
    Disp_Update_Time(1);
}

void drawSettings(){
    /* Print the buttons */
    Adafruit_RA8875_graphicsMode();
    Adafruit_RA8875_fillScreen(RA8875_BLACK);
    Adafruit_RA8875_fillRoundRect(750,   0, 50, 150, 5, RA8875_BLUE);

    /* Print the title */  
    Adafruit_RA8875_textMode();
    Adafruit_RA8875_textSetCursor(10, 130);
    Adafruit_RA8875_textTransparent(RA8875_CYAN);
    Adafruit_RA8875_textEnlarge(2);
    Adafruit_RA8875_textWrite("Settings", 8);
    
    /* Print the button labels */
    Adafruit_RA8875_textEnlarge(1);
    Adafruit_RA8875_textTransparent(RA8875_WHITE);
    Adafruit_RA8875_textSetCursor(757, 43);
    Adafruit_RA8875_textWrite("Back", 4);
}

void drawMessages(){
    int x;
    User *u;
    char str[50];
    
    /* Print the buttons */
    Adafruit_RA8875_graphicsMode();
    Adafruit_RA8875_fillScreen(RA8875_BLACK);
    Adafruit_RA8875_fillRoundRect(750,   0, 50, 150, 5, RA8875_BLUE);

    /* Print the title */  
    Adafruit_RA8875_textMode();
    Adafruit_RA8875_textSetCursor(10, 130);
    Adafruit_RA8875_textTransparent(RA8875_CYAN);
    Adafruit_RA8875_textEnlarge(2);
    Adafruit_RA8875_textWrite("Messages", 8);
    
    /* Print the list */
    // Will break for more than 13 users ***
    Adafruit_RA8875_textTransparent(RA8875_WHITE);
    x = 100;
    for (u = myself->users; u; u = u->next) {
        Adafruit_RA8875_textSetCursor(x, 10);
        sprintf(str, "%s(%d)", u->name, u->numMsgs);
        Adafruit_RA8875_textWrite(str, strlen(str));
        x += 50;
    }
        
    /* Print the button labels */
    Adafruit_RA8875_textEnlarge(1);
    Adafruit_RA8875_textTransparent(RA8875_WHITE);
    Adafruit_RA8875_textSetCursor(757, 43);
    Adafruit_RA8875_textWrite("Back", 4);
}

void drawInfo(){
    User *u;
    int x = 150;
    /* Print the buttons */
    Adafruit_RA8875_graphicsMode();
    Adafruit_RA8875_fillScreen(RA8875_BLACK);
    Adafruit_RA8875_fillRoundRect(750,   0, 50, 150, 5, RA8875_BLUE);

    /* Print the title */  
    Adafruit_RA8875_textMode();
    Adafruit_RA8875_textSetCursor(10, 95);
    Adafruit_RA8875_textTransparent(RA8875_CYAN);
    Adafruit_RA8875_textEnlarge(2);
    Adafruit_RA8875_textWrite("Information", 11);
    
    /* Print the list */
    // Will break for more than 13 users ***
    Adafruit_RA8875_textTransparent(RA8875_WHITE);
    Adafruit_RA8875_textSetCursor(100, 10);
    Adafruit_RA8875_textWrite("You", 3);
    for (u = myself->users; u; u = u->next) {
        Adafruit_RA8875_textSetCursor(x, 10);
        Adafruit_RA8875_textWrite(u->name, strlen(u->name));
        x += 50;
    }
    
    /* Print the button labels */
    Adafruit_RA8875_textEnlarge(1);
    Adafruit_RA8875_textTransparent(RA8875_WHITE);
    Adafruit_RA8875_textSetCursor(757, 43);
    Adafruit_RA8875_textWrite("Back", 4);
}

void drawNameEdit(){
}

void drawConvo(User *user){
    Message *m = user->msgs;
    int x = 100;
    
    curConvo = user;
    
    /* Print the buttons */
    Adafruit_RA8875_graphicsMode();
    Adafruit_RA8875_fillScreen(RA8875_BLACK);
    Adafruit_RA8875_fillRoundRect(750,   0, 50, 150, 5, RA8875_BLUE);
    Adafruit_RA8875_fillRoundRect(750, 330, 50, 150, 5, RA8875_BLUE);

    /* Print the title */  
    Adafruit_RA8875_textMode();
    Adafruit_RA8875_textTransparent(RA8875_CYAN);
    Adafruit_RA8875_textEnlarge(2);
    Adafruit_RA8875_textSetCursor(10, 95);
    Adafruit_RA8875_textWrite("Conversation", 12);
    
    /* Print the conversation */
    Adafruit_RA8875_textTransparent(RA8875_WHITE);
    Adafruit_RA8875_textEnlarge(1);
    int lines = 0;
    if (m) {
        // Count the messages we can display
        do {
            m = m->prev;
            int tempLines = m->msgLen / CHAR_PER_LINE + 1;
            if (tempLines + lines > MAX_LINES) {
                m = m->next;
                break;
            }
            lines += tempLines;
        } while(m != user->msgs);
        
        // Display the messages.
        do {
            if (m->sent)
                Adafruit_RA8875_textColor(RA8875_WHITE, RA8875_GREEN);
            else
                Adafruit_RA8875_textColor(RA8875_WHITE, RA8875_BLUE);
            
            Adafruit_RA8875_textSetCursor(x, 0);
            if (m->msgLen)
                Adafruit_RA8875_textWrite(m->msg, m->msgLen);
            else
                Adafruit_RA8875_textWrite(" ", 1);
            
            x += (int)(m->msgLen / CHAR_PER_LINE + 1) * PIX_PER_LINE;
            m = m->next;
        } while(m != user->msgs);
    }
    
    /* Print the button labels */
    Adafruit_RA8875_textEnlarge(1);
    Adafruit_RA8875_textTransparent(RA8875_WHITE);
    Adafruit_RA8875_textSetCursor(757, 43);
    Adafruit_RA8875_textWrite("Back", 4);
    Adafruit_RA8875_textSetCursor(757, 349);
    Adafruit_RA8875_textWrite("Compose", 7);
}

void drawCompose(){
    int x = 588, y = 3, i;
    char str[26];
    
    /* Print the buttons */
    Adafruit_RA8875_graphicsMode();
    Adafruit_RA8875_fillScreen(RA8875_BLACK);
    Adafruit_RA8875_fillRoundRect(582,   0, 218, 480, 5, RA8875_GREEN);
    Adafruit_RA8875_fillRoundRect(750,   0,  50, 150, 5, RA8875_BLUE);
    Adafruit_RA8875_fillRoundRect(750, 330,  50, 150, 5, RA8875_BLUE);
    
    /* Print the keyboard */
    for (i = 0; i < 10; i++) {
        Adafruit_RA8875_fillRoundRect(x, y, 48, 42, 5, RA8875_WHITE);
        y += 48;
    }
    for (i = 0, x = 642, y = 24; i < 9; i++) {
        Adafruit_RA8875_fillRoundRect(x, y, 48, 42, 5, RA8875_WHITE);
        Adafruit_RA8875_fillRoundRect(x + 54, y, 48, 42, 5, RA8875_WHITE);
        y += 48;
    }
    Adafruit_RA8875_fillRoundRect(750, 160, 50, 160, 5, RA8875_WHITE);
    
    /* Print the key labels */
    Adafruit_RA8875_textMode();
    Adafruit_RA8875_textEnlarge(1);
    Adafruit_RA8875_textTransparent(RA8875_BLACK);
    Adafruit_RA8875_textSetCursor(595, 0);
    Adafruit_RA8875_textWrite(" Q  W  E  R  T  Y  U  I  O  P", 29);
    Adafruit_RA8875_textSetCursor(649, 38);
    Adafruit_RA8875_textWrite("A  S  D  F  G  H  J  K  L", 25);
    Adafruit_RA8875_textSetCursor(703, 38);
    sprintf(str, "%c  Z  X  C  V  B  N  M  %c", 30, 17);
    Adafruit_RA8875_textWrite(str, 25);
    Adafruit_RA8875_textSetCursor(757, 198);
    Adafruit_RA8875_textWrite("Space", 5);
    
    /* Print the temp message */
    if (curConvo->tempMsg.msgLen) {
        Adafruit_RA8875_textEnlarge(2);
        Adafruit_RA8875_textTransparent(RA8875_WHITE);
        Adafruit_RA8875_textSetCursor(0, 0);
        Adafruit_RA8875_textWrite(curConvo->tempMsg.msg, curConvo->tempMsg.msgLen);
    }
    
    /* Print the button labels */
    Adafruit_RA8875_textEnlarge(1);
    Adafruit_RA8875_textSetCursor(757, 43);
    Adafruit_RA8875_textWrite("Back", 4);
    Adafruit_RA8875_textSetCursor(757, 373);
    Adafruit_RA8875_textWrite("Send", 4);
}

void drawDetails(void *user){
    char str[50];
    
    curDetails = user;
    
    /* Print the buttons */
    Adafruit_RA8875_graphicsMode();
    Adafruit_RA8875_fillScreen(RA8875_BLACK);
    Adafruit_RA8875_fillRoundRect(300,   0, 50, 150, 5, RA8875_BLUE);
    Adafruit_RA8875_fillRoundRect(750,   0, 50, 150, 5, RA8875_BLUE);

    /* Print the title */  
    Adafruit_RA8875_textMode();
    Adafruit_RA8875_textSetCursor(10, 95);
    Adafruit_RA8875_textTransparent(RA8875_CYAN);
    Adafruit_RA8875_textEnlarge(2);
    Adafruit_RA8875_textWrite("User Details", 12);
    
    /* Print the user info */
    if (user == myself) {
        //Adafruit_RA8875_textEnlarge(1);
        Adafruit_RA8875_textTransparent(RA8875_WHITE);
        Adafruit_RA8875_textSetCursor(100, 10);
        Adafruit_RA8875_textWrite(myself->name, strlen(myself->name));
    
        sprintf(str, "ID: %lu", myself->id);
        Adafruit_RA8875_textSetCursor(150, 10);
        Adafruit_RA8875_textWrite(str, strlen(str));

        Adafruit_RA8875_textSetCursor(200, 10);
        // Easy way to test for a valid position
        if (myself->rmc.latDir) {
            Adafruit_RA8875_textWrite("Position:", 9);
            Adafruit_RA8875_textSetCursor(250, 10);
            sprintf(str, "%0.4f%c, %0.4f%c", myself->rmc.lat, 248, myself->rmc.lon, 248);
            Adafruit_RA8875_textWrite(str, strlen(str));
        }
        else {
            Adafruit_RA8875_textWrite("Unknown position", 16);
        }
    }
    else if (user){
        User *u = (User*)user;
        //Adafruit_RA8875_textEnlarge(1);
        Adafruit_RA8875_textTransparent(RA8875_WHITE);
        Adafruit_RA8875_textSetCursor(100, 10);
        Adafruit_RA8875_textWrite(u->name, strlen(u->name));
    
        sprintf(str, "ID: %llu", u->uniqueID);
        Adafruit_RA8875_textSetCursor(150, 10);
        Adafruit_RA8875_textWrite(str, strlen(str));

        Adafruit_RA8875_textSetCursor(200, 10);
        // Easy way to test for a valid position
        if (u->pos.latDir) {
            Adafruit_RA8875_textWrite("Position:", 9);
            Adafruit_RA8875_textSetCursor(250, 10);
            sprintf(str, "%0.4f%c, %0.4f%c", u->pos.lat, 248, u->pos.lon, 248);
            Adafruit_RA8875_textWrite(str, strlen(str));
        }
        else {
            Adafruit_RA8875_textWrite("Unknown position", 16);
        }
    }
    else {
        // Shouldn't happen, but we'll get a warning
        // if it does
        Adafruit_RA8875_textSetCursor(100, 400);
        Adafruit_RA8875_textTransparent(RA8875_RED);
        Adafruit_RA8875_textWrite("NULL USER", 9);
    }
    
    /* Print the button labels */
    Adafruit_RA8875_textEnlarge(1);
    Adafruit_RA8875_textTransparent(RA8875_WHITE);
    Adafruit_RA8875_textSetCursor(307, 12);
    Adafruit_RA8875_textWrite("Messages", 8);
    Adafruit_RA8875_textSetCursor(757, 43);
    Adafruit_RA8875_textWrite("Back", 4);
}

/*
    Draws the requested menu and updates curMenu accordingly.
    m is the destination menu.
    arg is an optional argument needed for the menu.
*/
void goToMenu(Menu m, void *arg) {
    curMenu = m;
    
    switch(m) {
        case MENU_HOME:
            drawHome();
            break;
        case MENU_SETTINGS:
            drawSettings();
            break;
        case MENU_MESSAGES:
            drawMessages();
            break;
        case MENU_INFO:
            drawInfo();
            break;
        case MENU_NAME_EDIT:
            drawNameEdit();
            break;
        case MENU_CONVERSATION:
            drawConvo(arg);
            break;
        case MENU_COMPOSE:
            drawCompose();
            break;
        case MENU_INFO_DETAILS:
            drawDetails(arg);
            break;
        default:
            PC_PutString("Invalid menu\r\n");
            break;
    }
}