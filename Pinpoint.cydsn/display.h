#include <stdio.h>
#include <math.h>
#include "Adafruit_RA8875.h"
#include "users.h"
#include "nmea.h"
#include "main.h"

// Since all buttons are the same size I can use this macro
#define BUTTON_HIT(_T_X, _T_Y, _B_X, _B_Y)\
   (_T_X >= _B_X && _T_X <= _B_X + 150 && _T_Y >= _B_Y && _T_Y <= _B_Y + 50)

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

static Menu previous[9] = {MENU_HOME, MENU_HOME, MENU_HOME, MENU_HOME, 
    MENU_SETTINGS, MENU_MESSAGES, MENU_CONVERSATION, MENU_INFO};

Menu curMenu;

Self *myself;

/* "Public" functions */
void Disp_FurtherInit(Self *me);
void Disp_Refresh_Map();
void Disp_Update_Time();
int  Disp_Get_Touch(uint16 *x, uint16 *y);
void Disp_touchResponse(int x, int y);

/* "Private" functions */
void drawHome();
void drawSettings();
void drawMessages();
void drawInfo();
void drawNameEdit();
void drawConvo(User *user);
void drawCompose(User *user);
void drawDetails(void *user);
void goToMenu(Menu m, void *arg);
#endif