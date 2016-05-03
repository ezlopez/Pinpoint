#include <stdio.h>
#include <math.h>
#include "Adafruit_RA8875.h"
#include "users.h"
#include "nmea.h"

#ifndef __DISPLAY_H
#define __DISPLAY_H    

void Disp_FurtherInit(char *name);
void Disp_Refresh_Screen(Position *my_pos, User *list);
void Disp_drawCanvas(char *name);
void Disp_Update_Time(int utc);
void Disp_Clear_Map();
#endif