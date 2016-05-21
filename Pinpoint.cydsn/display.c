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
    // Back is on all menus but home
    if (curMenu != MENU_HOME && (BUTTON_HIT(x, y, 750, 0))) {
        PC_PutString("Back button\r\n");
        goToMenu(previous[curMenu], NULL);
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
            break;
        case MENU_INFO:
            break;
        case MENU_NAME_EDIT:
            break;
        case MENU_CONVERSATION:
            break;
        case MENU_COMPOSE:
            break;
        case MENU_INFO_DETAILS:
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
*/
void Disp_Update_Time() {
    int utc = myself->rmc.utc;
    static int prevMin = -1;
    char text[13];
    int hours = utc / 10000;
    int minutes = (utc % 10000) / 100;
    char *meridiem;
    char *format;
    
    if (minutes != prevMin) {
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
    Disp_Update_Time();
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
    
    /* Print the button labels */
    Adafruit_RA8875_textEnlarge(1);
    Adafruit_RA8875_textTransparent(RA8875_WHITE);
    Adafruit_RA8875_textSetCursor(757, 43);
    Adafruit_RA8875_textWrite("Back", 4);
}

void drawInfo(){
    /* Print the buttons */
    Adafruit_RA8875_graphicsMode();
    Adafruit_RA8875_fillScreen(RA8875_BLACK);
    Adafruit_RA8875_fillRoundRect(750,   0, 50, 150, 5, RA8875_BLUE);

    /* Print the title */  
    Adafruit_RA8875_textMode();
    Adafruit_RA8875_textSetCursor(10, 115);
    Adafruit_RA8875_textTransparent(RA8875_CYAN);
    Adafruit_RA8875_textEnlarge(2);
    Adafruit_RA8875_textWrite("Information", 11);
    
    /* Print the button labels */
    Adafruit_RA8875_textEnlarge(1);
    Adafruit_RA8875_textTransparent(RA8875_WHITE);
    Adafruit_RA8875_textSetCursor(757, 43);
    Adafruit_RA8875_textWrite("Back", 4);
}

void drawNameEdit(){
}

void drawConvo(User *user){
}

void drawCompose(User *user){
}

void drawDetails(void *user){
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
            drawCompose(arg);
            break;
        case MENU_INFO_DETAILS:
            drawDetails(arg);
            break;
        default:
            PC_PutString("Invalid menu\r\n");
            break;
    }
}