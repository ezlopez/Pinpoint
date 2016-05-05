#include "display.h"
#include <project.h>

void Disp_FurtherInit(char *name) {
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
    
    Disp_drawCanvas(name);
    Disp_Update_Time(0);
    Disp_Refresh_Screen(NULL, NULL);

  PC_PutString("Finished TFT init\r\n");
}

/*
    Updates the map to show any changes in User positions.
    If there are no users, it prints an empty map with default
    max distance of 2.0 miles.
*/
void Disp_Refresh_Screen(Position *my_pos, User *list) {
     // If you have more than 30 users, fix this
    double latDist[30], lonDist[30], totDist;
    int i, numInvalid = 0;
    double maxDist = 6.0/4; // So that the default is 2.0
    double transX, transY;
    char text[100];
    User *u = list;
    
    Disp_Clear_Map();
    
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
    
    if (!list) {
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
    
    
    // Paint all the users
    for (u = list, i = 0; u; i++, u = u->next) {
        if (u->pos.latDir != 0 && my_pos->latDir) {
            transX = 240 * latDist[i] / maxDist;
            transY = 240 * lonDist[i] / maxDist;
            Adafruit_RA8875_fillCircle(500 + transX, 240 - transY, 7, u->uniqueID & RA8875_WHITE);
        }
        else
            ++numInvalid;
    }
    
    if (numInvalid) {
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
void Disp_Update_Time(int utc) {
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
    Clears out and redraws the bullseye map.
*/
void Disp_Clear_Map() {
    /* Switch to graphics mode and print the map */
    Adafruit_RA8875_graphicsMode();
    
    // Clear the map area
    Adafruit_RA8875_rectHelper(200, 0, 760, 479, RA8875_BLACK, 1);
    
    // Redraw the bullseye
    Adafruit_RA8875_drawCircle(500, 240, 239, RA8875_WHITE);
    Adafruit_RA8875_drawCircle(500, 240, 160, RA8875_WHITE);
    Adafruit_RA8875_drawCircle(500, 240, 80, RA8875_WHITE);
    Adafruit_RA8875_fillCircle(500, 240, 7, RA8875_WHITE);
}

/*
    Draws a black background with the header to refresh the screen.
*/
void Disp_drawCanvas(char *name) {
    char text[50];
    
    // With hardware accelleration this is instant
    Adafruit_RA8875_fillScreen(RA8875_BLACK);

    /* Switch to text mode and print the header */  
    Adafruit_RA8875_textMode();
    Adafruit_RA8875_setOrientation(1);
    Adafruit_RA8875_textSetCursor(10, 120);
    Adafruit_RA8875_textTransparent(RA8875_CYAN);
    Adafruit_RA8875_textEnlarge(2);
    Adafruit_RA8875_textWrite("Pinpoint!", 9);
    Adafruit_RA8875_textEnlarge(1);
    Adafruit_RA8875_textSetCursor(760, 10);
    Adafruit_RA8875_textTransparent(RA8875_YELLOW);
    Adafruit_RA8875_textWrite(name, strlen(name));
}