#include <project.h>
#include <stdlib.h>
#include "cyapicallbacks.h"
#include "users.h"
#include "display.h"
#include "main.h"

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
uint8  xbBuffer[200];
uint8  xbUpdate[200];
uint8 xbReady = 0;

// TFT display variables
CY_ISR_PROTO(TFT_INTER);
CY_ISR_PROTO(TFT_REFRESH_INTER);
uint8 refreshReady = 0;

// List of the users we have seen on the network
User *users = NULL;

// Our own information
Self me;

int main() {
char test[200];
XBEE_Header *dr = (XBEE_Header *)xbUpdate;
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
    TFT_ISR_StartEx(TFT_INTER);
    Display_Refresh_Timer_Start();
    Display_Refresh_StartEx(TFT_REFRESH_INTER);

    // Setting up our own user data
    memset(&me, 0, sizeof(me));
    strncpy(me.name, "Alfred", 7); // Probably shouldn't hard-code***
    CyGetUniqueId(&me.id);
    
    CyGlobalIntEnable;
    
    Disp_FurtherInit(me.name);
    GPS_FurtherInit();

    gpsReady = broadcastReady = refreshReady = pcReady =  xbReady = 0;
    
    while(1) {
        if (pcReady) {
            PC_PutString("Ack pc\n");
            pcReady = 0;
        }
        if (gpsReady) {
            PC_PutString(gpsString);
            logGPSdata();
            gpsReady = 0;
        }
        if (xbReady) {
            logXBdata();
            xbReady = 0;
        }
        if (refreshReady) {
            Disp_Refresh_Screen((Position*)&(me.rmc.lat), users);
            Disp_Update_Time(me.rmc.utc);
            refreshReady = 0;
        }
        if (broadcastReady) {
            broadcastPosition();
            broadcastReady = 0;
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

/*
    Parses the message and enacts the appropriate action depending
    on the type of data in the message.
*/
void logXBdata() {
    XBEE_Header *hdr = (XBEE_Header *)xbUpdate;
    
    if (hdr->type == POSITION) {
        updateUsers(&users, (User*)(xbUpdate + sizeof(XBEE_Header)));
    }
    else {
        // Currently not implemented ***
        PC_PutString("Incorrect XB message type\r\n");
    }
}

void broadcastPosition() {
    uint8 message[sizeof(XBEE_Header) + sizeof(User)];
    XBEE_Header *hdr = (XBEE_Header*)message;
    User *u = (User*)(message + sizeof(XBEE_Header));
    
    Broadcast_Timer_ReadStatusRegister(); // Needed to clear interrupt output
    
    // Filling the header info
    hdr->destID = 0;
    hdr->type = POSITION;
    hdr->dataLen = sizeof(User);
    
    // Fill the user info
    u->uniqueID = me.id;
    strcpy(u->name, me.name);
    u->utc = me.rmc.utc;
    memcpy(&(u->pos), &(me.rmc.lat), sizeof(Position)); // Works because packed
    u->pdop = me.gsa.pdop;
    u->groundSpeed = me.rmc.groundSpeed; // In knots
    u->groundCourse = me.rmc.groundCourse; // In degrees
    
    XB_PutArray(message, sizeof(XBEE_Header) + sizeof(User));
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
    
    while(xbBufLen < sizeof(XBEE_Header) + hdr->dataLen) {
        if (XB_GetRxBufferSize()) {
            xbBuffer[xbBufLen++] = XB_ReadRxData();
        }
        else {
            CyGlobalIntEnable;
            return;
        }
    }
    
    // Transfering data to the second buffer
    xbReady = 1;
    memcpy(xbUpdate, xbBuffer, xbBufLen);
    xbBufLen = 0;

    CyGlobalIntEnable;
}

CY_ISR(TFT_INTER) {
    PC_PutString("Got display interrupt\r\n");
}

CY_ISR(TFT_REFRESH_INTER) {
    refreshReady = 1;
}

CY_ISR(BRDCST_LOC) {
    broadcastReady = 1;
}