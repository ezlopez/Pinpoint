#include <stdio.h>
#include <math.h>
#include "Adafruit_RA8875.h"
#include "users.h"
#include "nmea.h"

#ifndef __DISPLAY_H
#define __DISPLAY_H
    
/*
    Calibration follows the following equations:
    x_screen = (x_touch * A) + (y_touch * B) + C
    y_screen = (x_touch * D) + (y_touch * E) + F
    
    Where tsCalCoeff = {A, B, C, D, E, F}
    is calibrated during FurtherInit()
*/
float tsCalCoeff[6];

/*
    List of menus. <previous> defines which menu you should go to when
    hitting the "back" button.
*/
typedef enum {MENU_HOME, MENU_SETTINGS, MENU_MESSAGES, MENU_INFO, MENU_NAME_EDIT, 
    MENU_CONVERSATION, MENU_COMPOSE, MENU_INFO_DETAILS} Menu;

static int previous[9] = {MENU_HOME, MENU_HOME, MENU_HOME, MENU_HOME, 
    MENU_SETTINGS, MENU_MESSAGES, MENU_CONVERSATION, MENU_INFO};

int curMenu;

void Disp_FurtherInit(char *name);
void Disp_Refresh_Screen(Position *my_pos, User *list);
void Disp_drawCanvas(char *name);
void Disp_Update_Time(int utc);
int  Disp_Get_Touch(uint16 *x, uint16 *y);
void Disp_Clear_Map();
#endif