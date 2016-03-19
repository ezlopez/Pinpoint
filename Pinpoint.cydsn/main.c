#include <project.h>
#include <cyapicallbacks.h>
#include <stdlib.h>

#include "nmea.h"
#include "users.h"

// Enum for interrupt differentiation
typedef enum {GPS, XB, PC} UART_INTER;

// GPS RX buffer variables
uint8 gpsBufLen = 0;
char  gpsBuffer[500];
char  gpsString[500];
uint8 gpsReady = 0;

// PC RX buffer variables
uint8 pcBufLen = 0;
char  pcBuffer[100];
uint8 pcReady = 0;

// XB RX buffer variables
uint8 xbBufLen = 0;
char  xbBuffer[100];
User  xbUpdate;
uint8 xbReady = 0;

// User list, the first entry is this device
User users;

int main() {
char send[100];
    // Initializing GPS memory
    uint8 *gpsInfo = malloc(sizeofLargest());
    
    // Initializing GPS UART Module
    GPS_Start();
    GPS_TX_SetDriveMode(GPS_TX_DM_STRONG); // To reduce initial glitch output
    
    // Initializing XBee UART Module
    XB_Start();
    XB_TX_SetDriveMode(XB_TX_DM_STRONG); // To reduce initial glitch output
    
    // Initializing GPS UART Module
    PC_Start();
    PC_TX_SetDriveMode(PC_TX_DM_STRONG); // To reduce initial glitch output

    // Clearing out the user list and naming ourself
    cymemset(&users, 0, sizeof(User));
    strncpy(users.name, "Default", strlen("Default"));
    CyGetUniqueId((uint32*)&users.uniqueID); // Cheating a little bit
    
    CyGlobalIntEnable;

    while(1) {
        if (pcReady) {
            PC_PutString("Ack pc\n");
            pcReady = 0;
        }
        if (gpsReady) {
            PC_PutString(gpsString);
            sprintf(send, "GPS String: %d\r\n", parseNMEA(gpsString, gpsInfo));
            PC_PutString(send);
            XB_PutString(send);
            gpsReady = 0;
        }
        if (xbReady) {
            PC_PutString("Received XB packet: ");
            PC_PutString((char*)&xbUpdate);
            PC_PutString("\r\n");
            xbReady = 0;
        }
    }
}

void GPS_RXISR_ExitCallback() {
    CYGlobalIntDisable;
    CR_Write(~CR_Read());
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
    CYGlobalIntEnable;
}

// Really only used for testing. The PC will not be connected during operation
void PC_RXISR_ExitCallback() {
    CYGlobalIntDisable;
    CR_Write(~CR_Read());
    // Keep adding chars to the buffer until you
    // exhaust the internal buffer
    // Will break for more than one string at a time ********************
    while(PC_GetRxBufferSize()) {
        if ((gpsBuffer[gpsBufLen++] = PC_GetChar()) == '\n') { // End of packet
            gpsBuffer[gpsBufLen] = 0;
            gpsReady = 1;
            strcpy(gpsString, gpsBuffer);
            gpsBufLen = 0;
        }
    }
    CYGlobalIntEnable;
}

void XB_RXISR_ExitCallback() {
    CYGlobalIntDisable;
    CR_Write(~CR_Read());
    // Keep adding chars to the buffer until you
    // exhaust the internal buffer
    // Will break for more than one user at a time ********************
    while(XB_GetRxBufferSize()) {
        xbBuffer[xbBufLen++] = XB_GetChar();
    }
    
    xbBuffer[xbBufLen] = 0;
    xbReady = 1;
    memcpy(&xbUpdate, xbBuffer, xbBufLen);
    xbBufLen = 0;
            
    CYGlobalIntEnable;
}