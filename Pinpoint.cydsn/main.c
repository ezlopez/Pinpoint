#include <project.h>
#include "nmea.h"

// Interrupt prototype defines
CY_ISR_PROTO(GPS_RX_INTER);
CY_ISR_PROTO(XB_RX_INTER);
CY_ISR_PROTO(PC_RX_INTER);

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

int main() {
    // Initializing GPS UART Module
    GPS_Start();
    GPS_TX_SetDriveMode(GPS_TX_DM_STRONG); // To reduce initial glitch output
    GPS_RX_INT_StartEx(GPS_RX_INTER);
    
    // Initializing XBee UART Module
    GPS_Start();
    XB_TX_SetDriveMode(XB_TX_DM_STRONG); // To reduce initial glitch output
    XB_RX_INT_StartEx(XB_RX_INTER);
    
    // Initializing GPS UART Module
    PC_Start();
    PC_TX_SetDriveMode(PC_TX_DM_STRONG); // To reduce initial glitch output
    PC_RX_INT_StartEx(PC_RX_INTER);

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

CY_ISR(GPS_RX_INTER) {
    buildBuffer(GPS, gpsBuffer, &gpsBufLen, &gpsReady);
}

CY_ISR(XB_RX_INTER) {
    buildBuffer(XB, xbBuffer, &xbBufLen, &xbReady);
}

CY_ISR(PC_RX_INTER) {
    buildBuffer(PC, pcBuffer, &pcBufLen, &pcReady);
}

