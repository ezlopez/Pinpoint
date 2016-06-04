#include <project.h>
#include <stdlib.h>
#include "cyapicallbacks.h"
#include "users.h"
#include "display.h"
#include "xbee.h"
#include "nmea.h"
#include "users.h"

void logGPSdata();

// GPS RX buffer variables
uint8 gpsBufLen = 0;
char  gpsBuffer[500];
char  gpsString[500];
uint8 gpsReady = 0;

// PC RX buffer variables
uint32 pcBufLen = 0;
char  pcBuffer[100];
uint8 pcReady = 0;

// XB RX buffer variables
CY_ISR_PROTO(BRDCST_LOC);
CY_ISR_PROTO(XBEE_RCV);
uint8 broadcastReady = 0;
uint16 xbBufLen = 0;
uint8  xbBuffer[300];
uint8  xbUpdate[300];
uint8 xbReady = 0;

// TFT display variables
CY_ISR_PROTO(TFT_REFRESH_INTER);
uint8 refreshReady = 0;

// Our own information
Self me;

int main() {
    uint16 x, y;
    
    // Initializing GPS UART Module
    GPS_CLK_Start();
    GPS_Start();
    GPS_TX_SetDriveMode(GPS_TX_DM_STRONG); // To reduce initial glitch output
    
    // Initializing XBee UART Module
    XB_Start();
    XB_TX_SetDriveMode(XB_TX_DM_STRONG); // To reduce initial glitch output
    Broadcast_Timer_Start();
    XB_Location_Broadcast_StartEx(BRDCST_LOC);
    XB_RX_ISR_StartEx(XBEE_RCV);
    
    // Initializing PC UART Module
    PC_Start();
    PC_TX_SetDriveMode(PC_TX_DM_STRONG); // To reduce initial glitch output
    
    // Initializing TFT display
    TFT_Start();
    Display_Refresh_Timer_Start();
    Display_Refresh_StartEx(TFT_REFRESH_INTER);

    // Setting up our own user data
    memset(&me, 0, sizeof(me));
    strncpy(me.name, "Alfred", 7); // Probably shouldn't hard-code***
    CyGetUniqueId(&me.id);
    
    CyGlobalIntEnable;
    
    Disp_FurtherInit(&me);
    GPS_FurtherInit();

    Display_Refresh_Timer_ReadStatusRegister();
    refreshReady = 0;
    Broadcast_Timer_ReadStatusRegister();
    broadcastReady = 0;
    
// **************
me.users = calloc(sizeof(User), 1);
memcpy(me.users->name, "Beth", 4);
me.users->uniqueID = 42069;
    
    while(1) {
        if (gpsReady) {
            //PC_PutString("GPS\r\n");
            logGPSdata();
            gpsReady = 0;
        }
        if (xbReady) {
            PC_PutString("\tXBEE User\r\n");
            logXBdata(&me, xbUpdate);
            Disp_Refresh_Map();
            xbReady = 0;
        }
        if (refreshReady) {
            //PC_PutString("Refresh\r\n");
            Display_Refresh_Timer_ReadStatusRegister();
            Disp_Update_Time(0);
            refreshReady = 0;
        }
        if (broadcastReady) {
            //PC_PutString("\t\tBroadcast\r\n");
            broadcastPosition(&me);
            broadcastReady = 0;
        }
        if (Disp_Get_Touch(&x, &y)) {
            Adafruit_RA8875_graphicsMode();
            Adafruit_RA8875_fillCircle(x, y, 5, RA8875_WHITE);
            Disp_touchResponse(x, y);
            CyDelay(250);
            Disp_Get_Touch(&x, &y);
        }
    }
}

void logGPSdata() {
    char gpsInfo[100]; // Largest structure is 98 bytes long.
    
    CyGlobalIntDisable;
    switch(parseNMEA(gpsString, gpsInfo)) {
        case GGA:
            memcpy(&me.gga, gpsInfo, sizeof(GGA_Str));
            break;
        case GSA:
            memcpy(&me.gsa, gpsInfo, sizeof(GSA_Str));
            break;
        case VTG:
            memcpy(&me.vtg, gpsInfo, sizeof(VTG_Str));
            break;
        case RMC:
            memcpy(&me.rmc, gpsInfo, sizeof(RMC_Str));
            break;
        case GSV:
            memcpy(&me.gsv, gpsInfo, sizeof(GSV_Str));
            break;
        case INVALID:
        default:
            PC_PutString("Invalid NMEA string.\r\n");
            break;
    }
    CyGlobalIntEnable;
}

void GPS_RXISR_ExitCallback() {
    CyGlobalIntDisable;
    // Keep adding chars to the buffer until you
    // exhaust the internal buffer
    // Will break for more than one string at a time ********************
    while(GPS_GetRxBufferSize()) {
        if ((gpsBuffer[gpsBufLen++] = GPS_GetChar()) == '\n') { // End of packet
            gpsBuffer[gpsBufLen] = 0;
            gpsReady = 1;
            strcpy(gpsString, gpsBuffer);
            gpsBufLen = 0;
        }
    }
    CyGlobalIntEnable;
}

// Really only used for testing. The PC will not be connected during operation
void PC_RXISR_ExitCallback() {
    CyGlobalIntDisable;
    // Keep adding chars to the buffer until you
    // exhaust the internal buffer
    // Will break for more than one string at a time ********************
    while(PC_GetRxBufferSize()) {
        if ((gpsBuffer[gpsBufLen++] = PC_GetChar()) == '\n') { // End of packet
            gpsBuffer[gpsBufLen] = 0;
            //gpsReady = 1;
            strcpy(gpsString, gpsBuffer);
            gpsBufLen = 0;
        }
    }
    CyGlobalIntEnable;
}

CY_ISR(XBEE_RCV){
    XBEE_Header *hdr = (XBEE_Header*) xbBuffer;

    CyGlobalIntDisable;    

    while(xbBufLen < sizeof(XBEE_Header)) {
        if (XB_GetRxBufferSize()) {
            xbBuffer[xbBufLen++] = XB_ReadRxData();
        }
        else {
            CyGlobalIntEnable;
            return;
        }
    }
    
    while(xbBufLen < sizeof(XBEE_Header) + XBEE_STR_SIZE[hdr->type]) {
        if (XB_GetRxBufferSize()) {
            if (hdr->destID == 0 || hdr->destID == me.id)
                xbBuffer[xbBufLen++] = XB_ReadRxData();
            else
                XB_ReadRxData();
        }
        else {
            CyGlobalIntEnable;
            return;
        }
    }
    
    // Transfering data to the second buffer
    if (hdr->destID == 0 || hdr->destID == me.id) {
        xbReady = 1;
        memcpy(xbUpdate, xbBuffer, xbBufLen);
    }
    xbBufLen = 0;

    CyGlobalIntEnable;
}

CY_ISR(TFT_REFRESH_INTER) {
    refreshReady = 1;
}

CY_ISR(BRDCST_LOC) {
    broadcastReady = 1;
}