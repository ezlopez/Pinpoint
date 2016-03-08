#include <project.h>
#include <cyapicallbacks.h>

#include "nmea.h"
#include "users.h"

// Enum for interrupt differentiation
typedef enum {GPS, XB, PC} UART_INTER;

// Ready flags for UART components
uint8 gpsReady = 0;
uint8 xbReady = 0;
uint8 pcReady = 0;

// GPS RX buffer variables
uint8 gpsBufLen = 0;
char  gpsBuffer[100];

// PC RX buffer variables
uint8 pcBufLen = 0;
char  pcBuffer[100];

// XB RX buffer variables
uint8 xbBufLen = 0;
char  xbBuffer[100];

// User list, the first entry is this device
User users;

int main() {
    // Initializing GPS UART Module
    GPS_Start();
    GPS_TX_SetDriveMode(GPS_TX_DM_STRONG); // To reduce initial glitch output
    
    // Initializing XBee UART Module
    GPS_Start();
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
    }
}

void buildBuffer(UART_INTER inter, char *buffer, uint8 *len, uint8 *ready) {
/*    char c;

    // Build the sentence if no other sentence is pending
    while (GPS_GetRxBufferSize() && !sentenceReady) {
        c = GPS_GetChar();
        if (rxBufLen || c == '$') { // Filters out random bytes
            rxBuffer[rxBufLen] = c;
            if (rxBufLen++ && c == '\n') { // End byte
                sentenceReady = 1;
                strncpy(sentence, rxBuffer, rxBufLen);
                sentence[rxBufLen] = 0;
                rxBufLen = 0;
            }
        }
        else {
            // Got random bytes
        }
    }*/
}

void GPS_RXISR_ExitCallback() {
    buildBuffer(GPS, gpsBuffer, &gpsBufLen, &gpsReady);
}

void PC_RXISR_ExitCallback() {
    buildBuffer(XB, xbBuffer, &xbBufLen, &xbReady);
}

void XB_RXISR_ExitCallback() {
    buildBuffer(PC, pcBuffer, &pcBufLen, &pcReady);
}