#include <cyapicallbacks.h>
#include <stdlib.h>

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

// List of the users we have seen on the network
User *users = NULL;

// Our own information
Self me;

void rxBuffering();

int main() {
char test[200];
XBEE_Header *hdr = (XBEE_Header *)xbUpdate;
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
    
    // Initializing GPS UART Module
    PC_Start();
    PC_TX_SetDriveMode(PC_TX_DM_STRONG); // To reduce initial glitch output

    // Setting up our own user data
    memset(&me, 0, sizeof(me));
    strncpy(me.name, "Alfred", 7); // Probably shouldn't hard-code***
    CyGetUniqueId(&me.id);
    
    CyGlobalIntEnable;
    
    GPS_FurtherInit();
    broadcastReady = 0;
    
    while(1) {
        /*if (GPS_GetRxBufferSize())
            GPS_RXISR_ExitCallback();
        if (PC_GetRxBufferSize())
            PC_RXISR_ExitCallback();
        if (XB_GetRxBufferSize())
            XB_RXISR_ExitCallback();*/
        
        if (pcReady) {
            PC_PutString("Ack pc\n");
            pcReady = 0;
        }
        if (gpsReady) {
            //PC_PutString(gpsString);
            logGPSdata();
            gpsReady = 0;
        }
        if (xbReady) {
            PC_PutString("XB Update:\r\n");
            //CyGlobalIntDisable;
            sprintf(test, "\t%s (%ld)\r\n\tType %d (%ld bytes)\r\n\tUTC %6.3f\r\n", hdr->name, hdr->id, hdr->type, hdr->dataLen, hdr->utc);
            PC_PutString(test);
            //CyGlobalIntEnable;
            xbReady = 0;
        }
        if (broadcastReady) {
            broadcastPosition();
            broadcastReady = 0;
        }
    }
}

/* Additional GPS setup:
 *    Temporarily kill all sentence output & clear buffer
 *    Set baudrate to 115200 (divider = 26 for 24 MHz clock)
 *    Set update rate to once per second
 *    Set NMEA output to be only RMC and GSA at every two seconds
 */
void GPS_FurtherInit() {
    CyDelay(1000);
    // Kill sentence output
    GPS_PutString("$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n");
    PC_PutString("Killing output\r\n");
    GPS_ClearRxBuffer();
    CyDelay(500); // Delay half a second to collect acknowledgement
    GPS_ClearRxBuffer();
    
    
    /*// Update the baud rate on the GPS chip and GPS UART module
    GPS_PutString("$PMTK251,115200*1F\r\n");
    GPS_CLK_SetDividerValue(26);
    PC_PutString("Changing baud rate\r\n");
    CyDelay(500); // Delay half a second to collect acknowledgement
    GPS_ClearRxBuffer();*/
    
    // Set GPS chip fix rate to 1 Hz
    GPS_PutString("$PMTK220,1000*1F\r\n");
    PC_PutString("Changing update rate\r\n");
    CyDelay(500); // Delay half a second to collect acknowledgement
    GPS_ClearRxBuffer();
    
    // Set GPS chip output to 0.5 Hz for RMC and GSA
    GPS_PutString("$PMTK314,0,2,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n");
    PC_PutString("Restarting output\r\n");
    CyDelay(500); // Delay half a second to collect acknowledgement
    GPS_ClearRxBuffer();
    gpsReady = 0;
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
            //PC_PutString("Invalid NMEA string.\r\n");
            break;
    }
    CyGlobalIntEnable;
}

CY_ISR(BRDCST_LOC) {
    broadcastReady = 1;
}

void broadcastPosition() {
    XBEE_Header hdr;
    XBEE_POSITION_MESSAGE pos;
    char message[150]; // Rough estimate
    
    Broadcast_Timer_ReadStatusRegister(); // Needed to clear interrupt output
    
    // Filling the header info
    memset(hdr.name, 0, 20);
    strcpy(hdr.name, "Alfred");//me.name);
    hdr.id = me.id;
    hdr.utc = me.rmc.utc;
    hdr.type = POSITION;
    hdr.dataLen = 0;//sizeof(XBEE_POSITION_MESSAGE);
    
    // Filling the position info
    //memcpy(&pos.pos, &me.rmc.lat, sizeof(Position)); // Only works because it's packed
    
    // Concatenating the header and payload
    //memcpy(message, &hdr, sizeof(hdr));
    //memcpy(message + sizeof(hdr), &pos, sizeof(pos));
    
    XB_PutArray((uint8*)&hdr, sizeof(hdr));// + sizeof(pos));
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
            PC_PutChar(xbBuffer[xbBufLen - 1]);
        }
        else {
            CyGlobalIntEnable;
            return;
        }
    }
   /* 
    while(xbBufLen < sizeof(XBEE_Header) + hdr->dataLen) {
        if (XB_GetRxBufferSize()) {
            xbBuffer[xbBufLen++] = XB_ReadRxData();
        }
        else {
            CyGlobalIntEnable;
            return;
        }
    }
*/
    
    // Transfering data to the second buffer
    xbReady = 1;
    memcpy(xbUpdate, xbBuffer, xbBufLen);
    xbBufLen = 0;

    CyGlobalIntEnable;
}