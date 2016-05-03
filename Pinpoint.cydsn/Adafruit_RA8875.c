/**************************************************************************/
/*!
    @file     Adafruit_RA8875.cpp
    @author   Limor Friend/Ladyada, K.Townsend/KTOWN for Adafruit Industries
    @license  BSD license, all text above and below must be included in
              any redistribution

 This is the library for the Adafruit RA8875 Driver board for TFT displays
 ---------------> http://www.adafruit.com/products/1590
 The RA8875 is a TFT driver for up to 800x480 dotclock'd displays
 It is tested to work with displays in the Adafruit shop. Other displays
 may need timing adjustments and are not guanteed to work.
 
 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source hardware
 by purchasing products from Adafruit!
 
 Written by Limor Fried/Ladyada for Adafruit Industries.
 BSD license, check license.txt for more information.
 All text above must be included in any redistribution.

    @section  HISTORY
    
    v1.0 - First release
*/

/*
    Revised for use with the PSOC 5LP
    Ezequiel Lopez - 04/22/16
*/
    
/**************************************************************************/
#include <Adafruit_RA8875.h>
#include <project.h>
#include "glcdfont.c"

// Many (but maybe not all) non-AVR board installs define macros
// for compatibility with existing PROGMEM-reading AVR code.
// Do our own checks and defines here for good measure...

#ifndef pgm_read_byte
 #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif
#ifndef pgm_read_word
 #define pgm_read_word(addr) (*(const unsigned short *)(addr))
#endif
#ifndef pgm_read_dword
 #define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#endif

// Pointers are a peculiar case...typically 16-bit on AVR boards,
// 32 bits elsewhere.  Try to accommodate both...

#if !defined(__INT_MAX__) || (__INT_MAX__ > 0xFFFF)
 #define pgm_read_pointer(addr) ((void *)pgm_read_dword(addr))
#else
 #define pgm_read_pointer(addr) ((void *)pgm_read_word(addr))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif

// This is the 'raw' display w/h - never changes
const int16_t WIDTH = 800;
const int16_t HEIGHT = 480;

// Display w/h as modified by current rotation
int16_t _width = 800;
int16_t _height = 480;
int16_t cursor_x = 0;
int16_t cursor_y = 0;

uint16_t textcolor = 0xFFFF;
uint16_t textbgcolor = 0xFFFF;
uint8_t textsize = 1;
uint8_t rotation;
uint8_t wrap = 1;   // If set, 'wrap' text at right edge of display
uint8_t cp437 = 0; // If set, use correct CP437 charset (default is off)
GFXfont *gfxFont = 0;
uint8_t _textScale;

/**************************************************************************/
/*!
      Initialises the LCD driver and any HW required by the display
      
      @args s[in] The display size, which can be either:
                  'RA8875_480x272' (4.3" displays) r
                  'RA8875_800x480' (5" and 7" displays)
*/
/**************************************************************************/
int Adafruit_RA8875_begin() {
    TFT_RST_Control_Write(0);
    CyDelay(100);
    TFT_RST_Control_Write(1);
    CyDelay(100);
  
    PC_PutString("Testing Reg\r\n");
    if (Adafruit_RA8875_readReg(0) != 0x75) {
        PC_PutString("Failed readReg(0)\r\n");
        return 0;
    }
    
    PC_PutString("Initializing Display\r\n");
    Adafruit_RA8875_initialize();
    TFT_CLOCK_SetDividerValue(3);
    
    return 1;
}

/************************* Initialization *********************************/

/**************************************************************************/
/*!
      Performs a SW-based reset of the RA8875
*/
/**************************************************************************/
void Adafruit_RA8875_softReset(void) {
    Adafruit_RA8875_writeCommand(RA8875_PWRR);
    Adafruit_RA8875_writeData(RA8875_PWRR_SOFTRESET);
    Adafruit_RA8875_writeData(RA8875_PWRR_NORMAL);
    CyDelay(1);
}

/**************************************************************************/
/*!
      Initialise the PLL
*/
/**************************************************************************/
void Adafruit_RA8875_PLLinit(void) {
    Adafruit_RA8875_writeReg(RA8875_PLLC1, RA8875_PLLC1_PLLDIV1 + 10);
    CyDelay(1);
    Adafruit_RA8875_writeReg(RA8875_PLLC2, RA8875_PLLC2_DIV4);
    CyDelay(1);
}

/**************************************************************************/
/*!
      Initialises the driver IC (clock setup, etc.)
*/
/**************************************************************************/
void Adafruit_RA8875_initialize(void) {
    Adafruit_RA8875_PLLinit();
    Adafruit_RA8875_writeReg(RA8875_SYSR, RA8875_SYSR_16BPP | RA8875_SYSR_MCU8);
    
    /* Timing values */
    uint8_t pixclk = RA8875_PCSR_PDATL | RA8875_PCSR_2CLK;
    uint8_t hsync_start = 32;
    uint8_t hsync_pw = 96;
    uint8_t hsync_finetune = 0;
    uint8_t hsync_nondisp = 26;
    uint8_t vsync_pw = 2; 
    uint16_t vsync_nondisp = 32;
    uint16_t vsync_start = 23;
    
    Adafruit_RA8875_writeReg(RA8875_PCSR, pixclk);
    CyDelay(1);
    
    /* Horizontal settings registers */
    Adafruit_RA8875_writeReg(RA8875_HDWR, (_width / 8) - 1);                          // H width: (HDWR + 1) * 8 = 480
    Adafruit_RA8875_writeReg(RA8875_HNDFTR, RA8875_HNDFTR_DE_HIGH + hsync_finetune);
    Adafruit_RA8875_writeReg(RA8875_HNDR, (hsync_nondisp - hsync_finetune - 2)/8);    // H non-display: HNDR * 8 + HNDFTR + 2 = 10
    Adafruit_RA8875_writeReg(RA8875_HSTR, hsync_start/8 - 1);                         // Hsync start: (HSTR + 1)*8 
    Adafruit_RA8875_writeReg(RA8875_HPWR, RA8875_HPWR_LOW + (hsync_pw/8 - 1));        // HSync pulse width = (HPWR+1) * 8
    
    /* Vertical settings registers */
    Adafruit_RA8875_writeReg(RA8875_VDHR0, (uint16_t)(_height - 1) & 0xFF);
    Adafruit_RA8875_writeReg(RA8875_VDHR1, (uint16_t)(_height - 1) >> 8);
    Adafruit_RA8875_writeReg(RA8875_VNDR0, vsync_nondisp-1);                          // V non-display period = VNDR + 1
    Adafruit_RA8875_writeReg(RA8875_VNDR1, vsync_nondisp >> 8);
    Adafruit_RA8875_writeReg(RA8875_VSTR0, vsync_start-1);                            // Vsync start position = VSTR + 1
    Adafruit_RA8875_writeReg(RA8875_VSTR1, vsync_start >> 8);
    Adafruit_RA8875_writeReg(RA8875_VPWR, RA8875_VPWR_LOW + vsync_pw - 1);            // Vsync pulse width = VPWR + 1
    
    /* Set active window X */
    Adafruit_RA8875_writeReg(RA8875_HSAW0, 0);                                        // horizontal start point
    Adafruit_RA8875_writeReg(RA8875_HSAW1, 0);
    Adafruit_RA8875_writeReg(RA8875_HEAW0, (uint16_t)(_width - 1) & 0xFF);            // horizontal end point
    Adafruit_RA8875_writeReg(RA8875_HEAW1, (uint16_t)(_width - 1) >> 8);
    
    /* Set active window Y */
    Adafruit_RA8875_writeReg(RA8875_VSAW0, 0);                                        // vertical start point
    Adafruit_RA8875_writeReg(RA8875_VSAW1, 0);  
    Adafruit_RA8875_writeReg(RA8875_VEAW0, (uint16_t)(_height - 1) & 0xFF);           // horizontal end point
    Adafruit_RA8875_writeReg(RA8875_VEAW1, (uint16_t)(_height - 1) >> 8);
    
    /* ToDo: Setup touch panel? */
    
    /* Clear the entire window */
    Adafruit_RA8875_writeReg(RA8875_MCLR, RA8875_MCLR_START | RA8875_MCLR_FULL);
    CyDelay(500); 
}

/************************* Text Mode ***********************************/

/**************************************************************************/
/*!
      Sets the display in text mode (as opposed to graphics mode)
*/
/**************************************************************************/
void Adafruit_RA8875_textMode(void) {
  /* Set text mode */
  Adafruit_RA8875_writeCommand(RA8875_MWCR0);
  uint8_t temp = Adafruit_RA8875_readData();
  temp |= RA8875_MWCR0_TXTMODE; // Set bit 7
  Adafruit_RA8875_writeData(temp);
  
  /* Select the internal (ROM) font */
  Adafruit_RA8875_writeCommand(0x21);
  temp = Adafruit_RA8875_readData();
  temp &= ~((1<<7) | (1<<5)); // Clear bits 7 and 5
  Adafruit_RA8875_writeData(temp);
}

/**************************************************************************/
/*!
      Sets the display in text mode (as opposed to graphics mode)
      
      @args x[in] The x position of the cursor (in pixels, 0..1023)
      @args y[in] The y position of the cursor (in pixels, 0..511)
*/
/**************************************************************************/
void Adafruit_RA8875_textSetCursor(uint16_t x, uint16_t y) {
  /* Set cursor location */
  Adafruit_RA8875_writeCommand(0x2A);
  Adafruit_RA8875_writeData(x & 0xFF);
  Adafruit_RA8875_writeCommand(0x2B);
  Adafruit_RA8875_writeData(x >> 8);
  Adafruit_RA8875_writeCommand(0x2C);
  Adafruit_RA8875_writeData(y & 0xFF);
  Adafruit_RA8875_writeCommand(0x2D);
  Adafruit_RA8875_writeData(y >> 8);
}

/**************************************************************************/
/*!
      Sets the fore and background color when rendering text
      
      @args foreColor[in] The RGB565 color to use when rendering the text
      @args bgColor[in]   The RGB565 colot to use for the background
*/
/**************************************************************************/
void Adafruit_RA8875_textColor(uint16_t foreColor, uint16_t bgColor) {
    uint8_t temp;
    /* Set Fore Color */
    Adafruit_RA8875_writeCommand(0x63);
    Adafruit_RA8875_writeData((foreColor & 0xf800) >> 11);
    Adafruit_RA8875_writeCommand(0x64);
    Adafruit_RA8875_writeData((foreColor & 0x07e0) >> 5);
    Adafruit_RA8875_writeCommand(0x65);
    Adafruit_RA8875_writeData((foreColor & 0x001f));
    
    /* Set Background Color */
    Adafruit_RA8875_writeCommand(0x60);
    Adafruit_RA8875_writeData((bgColor & 0xf800) >> 11);
    Adafruit_RA8875_writeCommand(0x61);
    Adafruit_RA8875_writeData((bgColor & 0x07e0) >> 5);
    Adafruit_RA8875_writeCommand(0x62);
    Adafruit_RA8875_writeData((bgColor & 0x001f));
    
    /* Clear transparency flag */
    Adafruit_RA8875_writeCommand(0x22);
    temp = Adafruit_RA8875_readData();
    temp &= ~(1<<6); // Clear bit 6
    Adafruit_RA8875_writeData(temp);
}

/**************************************************************************/
/*!
      Sets the fore color when rendering text with a transparent bg
      
      @args foreColor[in] The RGB565 color to use when rendering the text
*/
/**************************************************************************/
void Adafruit_RA8875_textTransparent(uint16_t foreColor) {
    uint8_t temp;
    /* Set Fore Color */
    Adafruit_RA8875_writeCommand(0x63);
    Adafruit_RA8875_writeData((foreColor & 0xf800) >> 11);
    Adafruit_RA8875_writeCommand(0x64);
    Adafruit_RA8875_writeData((foreColor & 0x07e0) >> 5);
    Adafruit_RA8875_writeCommand(0x65);
    Adafruit_RA8875_writeData((foreColor & 0x001f));
    
    /* Set transparency flag */
    Adafruit_RA8875_writeCommand(0x22);
    temp = Adafruit_RA8875_readData();
    temp |= (1<<6); // Set bit 6
    Adafruit_RA8875_writeData(temp);  
}

/**************************************************************************/
/*!
      Sets the text enlarge settings, using one of the following values:
      
      0 = 1x zoom
      1 = 2x zoom
      2 = 3x zoom
      3 = 4x zoom
      
      @args scale[in]   The zoom factor (0..3 for 1-4x zoom)
*/
/**************************************************************************/
void Adafruit_RA8875_textEnlarge(uint8_t scale) {
    uint8_t temp;
    if (scale > 3) scale = 3;
    
    /* Set font size flags */
    Adafruit_RA8875_writeCommand(0x22);
    temp = Adafruit_RA8875_readData();
    temp &= ~(0xF); // Clears bits 0..3
    temp |= scale << 2;
    temp |= scale;
    Adafruit_RA8875_writeData(temp);  
    
    _textScale = scale;
}

/**************************************************************************/
/*!
      Renders some text on the screen when in text mode
      
      @args buffer[in]    The buffer containing the characters to render
      @args len[in]       The size of the buffer in bytes
*/
/**************************************************************************/
void Adafruit_RA8875_textWrite(const char* buffer, uint16_t len) {
    uint16_t i;
    if (len == 0) 
        len = strlen(buffer);
        
    Adafruit_RA8875_writeCommand(RA8875_MRWC);
    for (i=0;i<len;i++) {
        Adafruit_RA8875_writeData(buffer[i]);
        // This delay is needed with textEnlarge(1) because
        // Teensy 3.X is much faster than Arduino Uno
        if (_textScale > 0)
        CyDelay(1);
    }
}

/*
    Sets the canvas orientation. Mode = 0 is landscape. Mode = 1 is portrait.
*/
void Adafruit_RA8875_setOrientation(uint8 mode) {
    if (mode == 0) {
        Adafruit_RA8875_writeReg(0x22, 0x00); // set text to landscape mode (no rotation)
        Adafruit_RA8875_writeReg(0x20, 0x00); // normal scan direction for Y (smallest to largest)
    }
    else if (mode == 1) {
        Adafruit_RA8875_writeReg(0x22, 0x10); // set text to portrait mode (rotate 90 degrees)
        Adafruit_RA8875_writeReg(0x20, 0x04); //reverse scan direction for Y (largest to smallest)
    }
}

/************************* Graphics ***********************************/

/**************************************************************************/
/*!
      Sets the display in graphics mode (as opposed to text mode)
*/
/**************************************************************************/
void Adafruit_RA8875_graphicsMode(void) {
    uint8_t temp;
    Adafruit_RA8875_writeCommand(RA8875_MWCR0);
    temp = Adafruit_RA8875_readData();
    temp &= ~RA8875_MWCR0_TXTMODE; // bit #7
    Adafruit_RA8875_writeData(temp);
}

/**************************************************************************/
/*!
      Waits for screen to finish by polling the status!
*/
/**************************************************************************/
int Adafruit_RA8875_waitPoll(uint8_t regname, uint8_t waitflag) {
    uint8_t temp;
    /* Wait for the command to finish */
    while (1) {
        temp = Adafruit_RA8875_readReg(regname);
        if (!(temp & waitflag))
            return 1;
        CyDelay(1);
    }
    return 0; // MEMEFIX: yeah i know, unreached! - add timeout?
}


/**************************************************************************/
/*!
      Sets the current X/Y position on the display before drawing
      
      @args x[in] The 0-based x location
      @args y[in] The 0-base y location
*/
/**************************************************************************/
void Adafruit_RA8875_setXY(uint16_t x, uint16_t y) {
  Adafruit_RA8875_writeReg(RA8875_CURH0, x);
  Adafruit_RA8875_writeReg(RA8875_CURH1, x >> 8);
  Adafruit_RA8875_writeReg(RA8875_CURV0, y);
  Adafruit_RA8875_writeReg(RA8875_CURV1, y >> 8);  
}

/**************************************************************************/
/*!
      HW accelerated function to push a chunk of raw pixel data
      
      @args num[in] The number of pixels to push
      @args p[in]   The pixel color to use
*/
/**************************************************************************/
void Adafruit_RA8875_pushPixels(uint32_t num, uint16_t p) {
    TFT_WriteTxData(RA8875_DATAWRITE);
    while (num--) {
        TFT_WriteTxData(p >> 8);
        TFT_WriteTxData(p);
    }
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void Adafruit_RA8875_fillRectEmpty(void) {
    Adafruit_RA8875_writeCommand(RA8875_DCR);
    Adafruit_RA8875_writeData(RA8875_DCR_LINESQUTRI_STOP | RA8875_DCR_DRAWSQUARE);
    Adafruit_RA8875_writeData(RA8875_DCR_LINESQUTRI_START | RA8875_DCR_FILL | RA8875_DCR_DRAWSQUARE);
}

/**************************************************************************/
/*!
      Draws a single pixel at the specified location

      @args x[in]     The 0-based x location
      @args y[in]     The 0-base y location
      @args color[in] The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void Adafruit_RA8875_drawPixel(int16_t x, int16_t y, uint16_t color) {
    Adafruit_RA8875_writeReg(RA8875_CURH0, x);
    Adafruit_RA8875_writeReg(RA8875_CURH1, x >> 8);
    Adafruit_RA8875_writeReg(RA8875_CURV0, y);
    Adafruit_RA8875_writeReg(RA8875_CURV1, y >> 8);  
    Adafruit_RA8875_writeCommand(RA8875_MRWC);
    
    TFT_WriteTxData(RA8875_DATAWRITE);
    TFT_WriteTxData(color >> 8);
    TFT_WriteTxData(color);
}

/**************************************************************************/
/*!
      Draws a HW accelerated line on the display
    
      @args x0[in]    The 0-based starting x location
      @args y0[in]    The 0-base starting y location
      @args x1[in]    The 0-based ending x location
      @args y1[in]    The 0-base ending y location
      @args color[in] The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void Adafruit_RA8875_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    /* Set X */
    Adafruit_RA8875_writeCommand(0x91);
    Adafruit_RA8875_writeData(x0);
    Adafruit_RA8875_writeCommand(0x92);
    Adafruit_RA8875_writeData(x0 >> 8);
    
    /* Set Y */
    Adafruit_RA8875_writeCommand(0x93);
    Adafruit_RA8875_writeData(y0); 
    Adafruit_RA8875_writeCommand(0x94);
    Adafruit_RA8875_writeData(y0 >> 8);
    
    /* Set X1 */
    Adafruit_RA8875_writeCommand(0x95);
    Adafruit_RA8875_writeData(x1);
    Adafruit_RA8875_writeCommand(0x96);
    Adafruit_RA8875_writeData((x1) >> 8);
    
    /* Set Y1 */
    Adafruit_RA8875_writeCommand(0x97);
    Adafruit_RA8875_writeData(y1); 
    Adafruit_RA8875_writeCommand(0x98);
    Adafruit_RA8875_writeData((y1) >> 8);
    
    /* Set Color */
    Adafruit_RA8875_writeCommand(0x63);
    Adafruit_RA8875_writeData((color & 0xf800) >> 11);
    Adafruit_RA8875_writeCommand(0x64);
    Adafruit_RA8875_writeData((color & 0x07e0) >> 5);
    Adafruit_RA8875_writeCommand(0x65);
    Adafruit_RA8875_writeData((color & 0x001f));
    
    /* Draw! */
    Adafruit_RA8875_writeCommand(RA8875_DCR);
    Adafruit_RA8875_writeData(0x80);
    
    /* Wait for the command to finish */
    Adafruit_RA8875_waitPoll(RA8875_DCR, RA8875_DCR_LINESQUTRI_STATUS);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void Adafruit_RA8875_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    Adafruit_RA8875_drawLine(x, y, x, y+h, color);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void Adafruit_RA8875_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    Adafruit_RA8875_drawLine(x, y, x+w, y, color);
}

/**************************************************************************/
/*!
      Draws a HW accelerated rectangle on the display

      @args x[in]     The 0-based x location of the top-right corner
      @args y[in]     The 0-based y location of the top-right corner
      @args w[in]     The rectangle width
      @args h[in]     The rectangle height
      @args color[in] The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void Adafruit_RA8875_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    Adafruit_RA8875_rectHelper(x, y, x+w, y+h, color, 0);
}

/**************************************************************************/
/*!
      Draws a HW accelerated filled rectangle on the display

      @args x[in]     The 0-based x location of the top-right corner
      @args y[in]     The 0-based y location of the top-right corner
      @args w[in]     The rectangle width
      @args h[in]     The rectangle height
      @args color[in] The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void Adafruit_RA8875_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    Adafruit_RA8875_rectHelper(x, y, x+w, y+h, color, 1);
}

/**************************************************************************/
/*!
      Fills the screen with the spefied RGB565 color

      @args color[in] The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void Adafruit_RA8875_fillScreen(uint16_t color) {  
    Adafruit_RA8875_rectHelper(0, 0, _width-1, _height-1, color, 1);
}

/**************************************************************************/
/*!
      Draws a HW accelerated circle on the display

      @args x[in]     The 0-based x location of the center of the circle
      @args y[in]     The 0-based y location of the center of the circle
      @args w[in]     The circle's radius
      @args color[in] The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void Adafruit_RA8875_drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    Adafruit_RA8875_circleHelper(x0, y0, r, color, 0);
}

/**************************************************************************/
/*!
      Draws a HW accelerated filled circle on the display

      @args x[in]     The 0-based x location of the center of the circle
      @args y[in]     The 0-based y location of the center of the circle
      @args w[in]     The circle's radius
      @args color[in] The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void Adafruit_RA8875_fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  Adafruit_RA8875_circleHelper(x0, y0, r, color, 1);
}

/**************************************************************************/
/*!
      Draws a HW accelerated triangle on the display

      @args x0[in]    The 0-based x location of point 0 on the triangle
      @args y0[in]    The 0-based y location of point 0 on the triangle
      @args x1[in]    The 0-based x location of point 1 on the triangle
      @args y1[in]    The 0-based y location of point 1 on the triangle
      @args x2[in]    The 0-based x location of point 2 on the triangle
      @args y2[in]    The 0-based y location of point 2 on the triangle
      @args color[in] The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void Adafruit_RA8875_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
  Adafruit_RA8875_triangleHelper(x0, y0, x1, y1, x2, y2, color, 0);
}

/**************************************************************************/
/*!
      Draws a HW accelerated filled triangle on the display

      @args x0[in]    The 0-based x location of point 0 on the triangle
      @args y0[in]    The 0-based y location of point 0 on the triangle
      @args x1[in]    The 0-based x location of point 1 on the triangle
      @args y1[in]    The 0-based y location of point 1 on the triangle
      @args x2[in]    The 0-based x location of point 2 on the triangle
      @args y2[in]    The 0-based y location of point 2 on the triangle
      @args color[in] The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void Adafruit_RA8875_fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
  Adafruit_RA8875_triangleHelper(x0, y0, x1, y1, x2, y2, color, 1);
}

/**************************************************************************/
/*!
      Draws a HW accelerated ellipse on the display

      @args xCenter[in]   The 0-based x location of the ellipse's center
      @args yCenter[in]   The 0-based y location of the ellipse's center
      @args longAxis[in]  The size in pixels of the ellipse's long axis
      @args shortAxis[in] The size in pixels of the ellipse's short axis
      @args color[in]     The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void Adafruit_RA8875_drawEllipse(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color) {
  Adafruit_RA8875_ellipseHelper(xCenter, yCenter, longAxis, shortAxis, color, 0);
}

/**************************************************************************/
/*!
      Draws a HW accelerated filled ellipse on the display

      @args xCenter[in]   The 0-based x location of the ellipse's center
      @args yCenter[in]   The 0-based y location of the ellipse's center
      @args longAxis[in]  The size in pixels of the ellipse's long axis
      @args shortAxis[in] The size in pixels of the ellipse's short axis
      @args color[in]     The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void Adafruit_RA8875_fillEllipse(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color) {
  Adafruit_RA8875_ellipseHelper(xCenter, yCenter, longAxis, shortAxis, color, 1);
}

/**************************************************************************/
/*!
      Draws a HW accelerated curve on the display

      @args xCenter[in]   The 0-based x location of the ellipse's center
      @args yCenter[in]   The 0-based y location of the ellipse's center
      @args longAxis[in]  The size in pixels of the ellipse's long axis
      @args shortAxis[in] The size in pixels of the ellipse's short axis
      @args curvePart[in] The corner to draw, where in clock-wise motion:
                            0 = 180-270°
                            1 = 270-0°
                            2 = 0-90°
                            3 = 90-180°
      @args color[in]     The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void Adafruit_RA8875_drawCurve(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint8_t curvePart, uint16_t color) {
    Adafruit_RA8875_curveHelper(xCenter, yCenter, longAxis, shortAxis, curvePart, color, 0);
}

/**************************************************************************/
/*!
      Draws a HW accelerated filled curve on the display

      @args xCenter[in]   The 0-based x location of the ellipse's center
      @args yCenter[in]   The 0-based y location of the ellipse's center
      @args longAxis[in]  The size in pixels of the ellipse's long axis
      @args shortAxis[in] The size in pixels of the ellipse's short axis
      @args curvePart[in] The corner to draw, where in clock-wise motion:
                            0 = 180-270°
                            1 = 270-0°
                            2 = 0-90°
                            3 = 90-180°
      @args color[in]     The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void Adafruit_RA8875_fillCurve(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint8_t curvePart, uint16_t color) {
  Adafruit_RA8875_curveHelper(xCenter, yCenter, longAxis, shortAxis, curvePart, color, 1);
}

/**************************************************************************/
/*!
      Helper function for higher level circle drawing code
*/
/**************************************************************************/
void Adafruit_RA8875_circleHelper(int16_t x0, int16_t y0, int16_t r, uint16_t color, int filled) {
    /* Set X */
    Adafruit_RA8875_writeCommand(0x99);
    Adafruit_RA8875_writeData(x0);
    Adafruit_RA8875_writeCommand(0x9a);
    Adafruit_RA8875_writeData(x0 >> 8);
    
    /* Set Y */
    Adafruit_RA8875_writeCommand(0x9b);
    Adafruit_RA8875_writeData(y0); 
    Adafruit_RA8875_writeCommand(0x9c);	   
    Adafruit_RA8875_writeData(y0 >> 8);
    
    /* Set Radius */
    Adafruit_RA8875_writeCommand(0x9d);
    Adafruit_RA8875_writeData(r);  
    
    /* Set Color */
    Adafruit_RA8875_writeCommand(0x63);
    Adafruit_RA8875_writeData((color & 0xf800) >> 11);
    Adafruit_RA8875_writeCommand(0x64);
    Adafruit_RA8875_writeData((color & 0x07e0) >> 5);
    Adafruit_RA8875_writeCommand(0x65);
    Adafruit_RA8875_writeData((color & 0x001f));
    
    /* Draw! */
    Adafruit_RA8875_writeCommand(RA8875_DCR);
    if (filled)
        Adafruit_RA8875_writeData(RA8875_DCR_CIRCLE_START | RA8875_DCR_FILL);
    else
        Adafruit_RA8875_writeData(RA8875_DCR_CIRCLE_START | RA8875_DCR_NOFILL);
    
    /* Wait for the command to finish */
    Adafruit_RA8875_waitPoll(RA8875_DCR, RA8875_DCR_CIRCLE_STATUS);
}

/**************************************************************************/
/*!
      Helper function for higher level rectangle drawing code
*/
/**************************************************************************/
void Adafruit_RA8875_rectHelper(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, int filled) {
    /* Set X */
    Adafruit_RA8875_writeCommand(0x91);
    Adafruit_RA8875_writeData(x);
    Adafruit_RA8875_writeCommand(0x92);
    Adafruit_RA8875_writeData(x >> 8);
    
    /* Set Y */
    Adafruit_RA8875_writeCommand(0x93);
    Adafruit_RA8875_writeData(y); 
    Adafruit_RA8875_writeCommand(0x94);	   
    Adafruit_RA8875_writeData(y >> 8);
    
    /* Set X1 */
    Adafruit_RA8875_writeCommand(0x95);
    Adafruit_RA8875_writeData(w);
    Adafruit_RA8875_writeCommand(0x96);
    Adafruit_RA8875_writeData((w) >> 8);
    
    /* Set Y1 */
    Adafruit_RA8875_writeCommand(0x97);
    Adafruit_RA8875_writeData(h); 
    Adafruit_RA8875_writeCommand(0x98);
    Adafruit_RA8875_writeData((h) >> 8);
    
    /* Set Color */
    Adafruit_RA8875_writeCommand(0x63);
    Adafruit_RA8875_writeData((color & 0xf800) >> 11);
    Adafruit_RA8875_writeCommand(0x64);
    Adafruit_RA8875_writeData((color & 0x07e0) >> 5);
    Adafruit_RA8875_writeCommand(0x65);
    Adafruit_RA8875_writeData((color & 0x001f));
    
    /* Draw! */
    Adafruit_RA8875_writeCommand(RA8875_DCR);
    if (filled)
        Adafruit_RA8875_writeData(0xB0);
    else
        Adafruit_RA8875_writeData(0x90);
    /* Wait for the command to finish */
    Adafruit_RA8875_waitPoll(RA8875_DCR, RA8875_DCR_LINESQUTRI_STATUS);
}

/**************************************************************************/
/*!
      Helper function for higher level triangle drawing code
*/
/**************************************************************************/
void Adafruit_RA8875_triangleHelper(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color, int filled) {
    /* Set Point 0 */
    Adafruit_RA8875_writeCommand(0x91);
    Adafruit_RA8875_writeData(x0);
    Adafruit_RA8875_writeCommand(0x92);
    Adafruit_RA8875_writeData(x0 >> 8);
    Adafruit_RA8875_writeCommand(0x93);
    Adafruit_RA8875_writeData(y0); 
    Adafruit_RA8875_writeCommand(0x94);
    Adafruit_RA8875_writeData(y0 >> 8);
    
    /* Set Point 1 */
    Adafruit_RA8875_writeCommand(0x95);
    Adafruit_RA8875_writeData(x1);
    Adafruit_RA8875_writeCommand(0x96);
    Adafruit_RA8875_writeData(x1 >> 8);
    Adafruit_RA8875_writeCommand(0x97);
    Adafruit_RA8875_writeData(y1); 
    Adafruit_RA8875_writeCommand(0x98);
    Adafruit_RA8875_writeData(y1 >> 8);
    
    /* Set Point 2 */
    Adafruit_RA8875_writeCommand(0xA9);
    Adafruit_RA8875_writeData(x2);
    Adafruit_RA8875_writeCommand(0xAA);
    Adafruit_RA8875_writeData(x2 >> 8);
    Adafruit_RA8875_writeCommand(0xAB);
    Adafruit_RA8875_writeData(y2); 
    Adafruit_RA8875_writeCommand(0xAC);
    Adafruit_RA8875_writeData(y2 >> 8);
    
    /* Set Color */
    Adafruit_RA8875_writeCommand(0x63);
    Adafruit_RA8875_writeData((color & 0xf800) >> 11);
    Adafruit_RA8875_writeCommand(0x64);
    Adafruit_RA8875_writeData((color & 0x07e0) >> 5);
    Adafruit_RA8875_writeCommand(0x65);
    Adafruit_RA8875_writeData((color & 0x001f));
    
    /* Draw! */
    Adafruit_RA8875_writeCommand(RA8875_DCR);
    if (filled)
        Adafruit_RA8875_writeData(0xA1);
    else
        Adafruit_RA8875_writeData(0x81);
    
    /* Wait for the command to finish */
    Adafruit_RA8875_waitPoll(RA8875_DCR, RA8875_DCR_LINESQUTRI_STATUS);
}

/**************************************************************************/
/*!
      Helper function for higher level ellipse drawing code
*/
/**************************************************************************/
void Adafruit_RA8875_ellipseHelper(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color, int filled) {
    /* Set Center Point */
    Adafruit_RA8875_writeCommand(0xA5);
    Adafruit_RA8875_writeData(xCenter);
    Adafruit_RA8875_writeCommand(0xA6);
    Adafruit_RA8875_writeData(xCenter >> 8);
    Adafruit_RA8875_writeCommand(0xA7);
    Adafruit_RA8875_writeData(yCenter); 
    Adafruit_RA8875_writeCommand(0xA8);
    Adafruit_RA8875_writeData(yCenter >> 8);
    
    /* Set Long and Short Axis */
    Adafruit_RA8875_writeCommand(0xA1);
    Adafruit_RA8875_writeData(longAxis);
    Adafruit_RA8875_writeCommand(0xA2);
    Adafruit_RA8875_writeData(longAxis >> 8);
    Adafruit_RA8875_writeCommand(0xA3);
    Adafruit_RA8875_writeData(shortAxis); 
    Adafruit_RA8875_writeCommand(0xA4);
    Adafruit_RA8875_writeData(shortAxis >> 8);
    
    /* Set Color */
    Adafruit_RA8875_writeCommand(0x63);
    Adafruit_RA8875_writeData((color & 0xf800) >> 11);
    Adafruit_RA8875_writeCommand(0x64);
    Adafruit_RA8875_writeData((color & 0x07e0) >> 5);
    Adafruit_RA8875_writeCommand(0x65);
    Adafruit_RA8875_writeData((color & 0x001f));
    
    /* Draw! */
    Adafruit_RA8875_writeCommand(0xA0);
    if (filled)
        Adafruit_RA8875_writeData(0xC0);
    else
        Adafruit_RA8875_writeData(0x80);
    
    /* Wait for the command to finish */
    Adafruit_RA8875_waitPoll(RA8875_ELLIPSE, RA8875_ELLIPSE_STATUS);
}

/**************************************************************************/
/*!
      Helper function for higher level curve drawing code
*/
/**************************************************************************/
void Adafruit_RA8875_curveHelper(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint8_t curvePart, uint16_t color, int filled) {
    /* Set Center Point */
    Adafruit_RA8875_writeCommand(0xA5);
    Adafruit_RA8875_writeData(xCenter);
    Adafruit_RA8875_writeCommand(0xA6);
    Adafruit_RA8875_writeData(xCenter >> 8);
    Adafruit_RA8875_writeCommand(0xA7);
    Adafruit_RA8875_writeData(yCenter); 
    Adafruit_RA8875_writeCommand(0xA8);
    Adafruit_RA8875_writeData(yCenter >> 8);
    
    /* Set Long and Short Axis */
    Adafruit_RA8875_writeCommand(0xA1);
    Adafruit_RA8875_writeData(longAxis);
    Adafruit_RA8875_writeCommand(0xA2);
    Adafruit_RA8875_writeData(longAxis >> 8);
    Adafruit_RA8875_writeCommand(0xA3);
    Adafruit_RA8875_writeData(shortAxis); 
    Adafruit_RA8875_writeCommand(0xA4);
    Adafruit_RA8875_writeData(shortAxis >> 8);
    
    /* Set Color */
    Adafruit_RA8875_writeCommand(0x63);
    Adafruit_RA8875_writeData((color & 0xf800) >> 11);
    Adafruit_RA8875_writeCommand(0x64);
    Adafruit_RA8875_writeData((color & 0x07e0) >> 5);
    Adafruit_RA8875_writeCommand(0x65);
    Adafruit_RA8875_writeData((color & 0x001f));
    
    /* Draw! */
    Adafruit_RA8875_writeCommand(0xA0);
    if (filled)
        Adafruit_RA8875_writeData(0xD0 | (curvePart & 0x03));
    else
        Adafruit_RA8875_writeData(0x90 | (curvePart & 0x03));
    
    /* Wait for the command to finish */
    Adafruit_RA8875_waitPoll(RA8875_ELLIPSE, RA8875_ELLIPSE_STATUS);
}

/************************* Mid Level ***********************************/

/**************************************************************************/
/*!

*/
/**************************************************************************/
void Adafruit_RA8875_GPIOX(int on) {
  if (on)
    Adafruit_RA8875_writeReg(RA8875_GPIOX, 1);
  else 
    Adafruit_RA8875_writeReg(RA8875_GPIOX, 0);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void Adafruit_RA8875_PWM1out(uint8_t p) {
  Adafruit_RA8875_writeReg(RA8875_P1DCR, p);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void Adafruit_RA8875_PWM2out(uint8_t p) {
  Adafruit_RA8875_writeReg(RA8875_P2DCR, p);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void Adafruit_RA8875_PWM1config(int on, uint8_t clock) {
    if (on)
        Adafruit_RA8875_writeReg(RA8875_P1CR, RA8875_P1CR_ENABLE | (clock & 0xF));
    else
        Adafruit_RA8875_writeReg(RA8875_P1CR, RA8875_P1CR_DISABLE | (clock & 0xF));
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void Adafruit_RA8875_PWM2config(int on, uint8_t clock) {
    if (on)
        Adafruit_RA8875_writeReg(RA8875_P2CR, RA8875_P2CR_ENABLE | (clock & 0xF));
    else
        Adafruit_RA8875_writeReg(RA8875_P2CR, RA8875_P2CR_DISABLE | (clock & 0xF));
}

/**************************************************************************/
/*!
      Enables or disables the on-chip touch screen controller
*/
/**************************************************************************/
void Adafruit_RA8875_touchEnable(int on) {
    if (on) {
        /* Enable Touch Panel (Reg 0x70) */
        Adafruit_RA8875_writeReg(RA8875_TPCR0, RA8875_TPCR0_ENABLE        | 
                                               RA8875_TPCR0_WAIT_4096CLK  |
                                               RA8875_TPCR0_WAKEDISABLE   | 
                                               RA8875_TPCR0_ADCCLK_DIV4); // 10mhz max!
        /* Set Auto Mode      (Reg 0x71) */
        Adafruit_RA8875_writeReg(RA8875_TPCR1, RA8875_TPCR1_AUTO    | 
                                            // RA8875_TPCR1_VREFEXT | 
                                               RA8875_TPCR1_DEBOUNCE);
        /* Enable TP INT */
        Adafruit_RA8875_writeReg(RA8875_INTC1, Adafruit_RA8875_readReg(RA8875_INTC1) | RA8875_INTC1_TP);
    } 
    else {
        /* Disable TP INT */
        Adafruit_RA8875_writeReg(RA8875_INTC1, Adafruit_RA8875_readReg(RA8875_INTC1) & ~RA8875_INTC1_TP);
        /* Disable Touch Panel (Reg 0x70) */
        Adafruit_RA8875_writeReg(RA8875_TPCR0, RA8875_TPCR0_DISABLE);
    }
}

/**************************************************************************/
/*!
      Checks if a touch event has occured
      
      @returns  1 is a touch event has occured (reading it via
                touchRead() will clear the interrupt in memory)
*/
/**************************************************************************/
int Adafruit_RA8875_touched(void) {
    if (Adafruit_RA8875_readReg(RA8875_INTC2) & RA8875_INTC2_TP)
        return 1;
    return 0;
}

/**************************************************************************/
/*!
      Reads the last touch event
      
      @args x[out]  Pointer to the uint16_t field to assign the raw X value
      @args y[out]  Pointer to the uint16_t field to assign the raw Y value
      
      @note Calling this function will clear the touch panel interrupt on
            the RA8875, resetting the flag used by the 'touched' function
*/
/**************************************************************************/
int Adafruit_RA8875_touchRead(uint16_t *x, uint16_t *y) {
    uint16_t tx, ty;
    uint8_t temp;
    
    tx = Adafruit_RA8875_readReg(RA8875_TPXH);
    ty = Adafruit_RA8875_readReg(RA8875_TPYH);
    temp = Adafruit_RA8875_readReg(RA8875_TPXYL);
    tx <<= 2;
    ty <<= 2;
    tx |= temp & 0x03;        // get the bottom x bits
    ty |= (temp >> 2) & 0x03; // get the bottom y bits
    
    *x = tx;
    *y = ty;
    
    /* Clear TP INT Status */
    Adafruit_RA8875_writeReg(RA8875_INTC2, RA8875_INTC2_TP);
    
    return 1;
}

/**************************************************************************/
/*!
      Turns the display on or off
*/
/**************************************************************************/
void Adafruit_RA8875_displayOn(int on) {
    if (on) 
        Adafruit_RA8875_writeReg(RA8875_PWRR, RA8875_PWRR_NORMAL | RA8875_PWRR_DISPON);
    else
        Adafruit_RA8875_writeReg(RA8875_PWRR, RA8875_PWRR_NORMAL | RA8875_PWRR_DISPOFF);
}

/**************************************************************************/
/*!
    Puts the display in sleep mode, or disables sleep mode if enabled
*/
/**************************************************************************/
void Adafruit_RA8875_sleep(int sleep) {
    if (sleep) 
        Adafruit_RA8875_writeReg(RA8875_PWRR, RA8875_PWRR_DISPOFF | RA8875_PWRR_SLEEP);
    else
        Adafruit_RA8875_writeReg(RA8875_PWRR, RA8875_PWRR_DISPOFF);
}

/************************* Low Level ***********************************/

/**************************************************************************/
/*!

*/
/**************************************************************************/
void  Adafruit_RA8875_writeReg(uint8_t reg, uint8_t val) {
    Adafruit_RA8875_writeCommand(reg);
    CyDelay(1);
    Adafruit_RA8875_writeData(val);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
uint8_t  Adafruit_RA8875_readReg(uint8_t reg) {
    uint8_t dat;
    Adafruit_RA8875_writeCommand(reg);
    CyDelay(1);
    return Adafruit_RA8875_readData();
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void  Adafruit_RA8875_writeData(uint8_t d) {
    uint8 data[2] = {RA8875_DATAWRITE, d};
    TFT_PutArray(data, 2);
    while (!(TFT_ReadTxStatus() & TFT_STS_SPI_DONE));
    while (TFT_GetRxBufferSize())
        TFT_ReadRxData();
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
uint8_t  Adafruit_RA8875_readData(void) {
    uint8 data[2] = {RA8875_DATAREAD, 0};
    TFT_PutArray(data, 2);
    while (!(TFT_ReadTxStatus() & TFT_STS_SPI_DONE));
    while (TFT_GetRxBufferSize() > 1)
        TFT_ReadRxData();
    return TFT_ReadRxData();
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void  Adafruit_RA8875_writeCommand(uint8_t d) {
    uint8 data[2] = {RA8875_CMDWRITE, d};
    TFT_PutArray(data, 2);
    while (!(TFT_ReadTxStatus() & TFT_STS_SPI_DONE));
    while (TFT_GetRxBufferSize())
        TFT_ReadRxData();
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
uint8_t  Adafruit_RA8875_readStatus(void) {
    uint8 data[2] = {RA8875_CMDREAD, 0};
    TFT_PutArray(data, 2);
    while (!(TFT_ReadTxStatus() & TFT_STS_SPI_DONE));
    while (TFT_GetRxBufferSize() > 1)
        TFT_ReadRxData();
    return TFT_ReadRxData();
}

/******* Default functions copied from the GFX class ************/

// Draw a rounded rectangle
void Adafruit_RA8875_drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    // smarter version
    Adafruit_RA8875_drawFastHLine(x+r  , y    , w-2*r, color); // Top
    Adafruit_RA8875_drawFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
    Adafruit_RA8875_drawFastVLine(x    , y+r  , h-2*r, color); // Left
    Adafruit_RA8875_drawFastVLine(x+w-1, y+r  , h-2*r, color); // Right
    // draw four corners
    Adafruit_RA8875_drawCircleHelper(x+r    , y+r    , r, 1, color);
    Adafruit_RA8875_drawCircleHelper(x+w-r-1, y+r    , r, 2, color);
    Adafruit_RA8875_drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
    Adafruit_RA8875_drawCircleHelper(x+r    , y+h-r-1, r, 8, color);
}

void Adafruit_RA8875_drawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color) {
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    if (cornername & 0x4) {
      Adafruit_RA8875_drawPixel(x0 + x, y0 + y, color);
      Adafruit_RA8875_drawPixel(x0 + y, y0 + x, color);
    }
    if (cornername & 0x2) {
      Adafruit_RA8875_drawPixel(x0 + x, y0 - y, color);
      Adafruit_RA8875_drawPixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      Adafruit_RA8875_drawPixel(x0 - y, y0 + x, color);
      Adafruit_RA8875_drawPixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      Adafruit_RA8875_drawPixel(x0 - y, y0 - x, color);
      Adafruit_RA8875_drawPixel(x0 - x, y0 - y, color);
    }
  }
}

// Fill a rounded rectangle
void Adafruit_RA8875_fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
  // smarter version
  Adafruit_RA8875_fillRect(x+r, y, w-2*r, h, color);

  // draw four corners
  Adafruit_RA8875_fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
  Adafruit_RA8875_fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
}

// Used to do circles and roundrects
void Adafruit_RA8875_fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color) {

  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;

    if (cornername & 0x1) {
      Adafruit_RA8875_drawFastVLine(x0+x, y0-y, 2*y+1+delta, color);
      Adafruit_RA8875_drawFastVLine(x0+y, y0-x, 2*x+1+delta, color);
    }
    if (cornername & 0x2) {
      Adafruit_RA8875_drawFastVLine(x0-x, y0-y, 2*y+1+delta, color);
      Adafruit_RA8875_drawFastVLine(x0-y, y0-x, 2*x+1+delta, color);
    }
  }
}

// Draw a 1-bit image (bitmap) at the specified (x,y) position from the
// provided bitmap buffer (must be PROGMEM memory) using the specified
// foreground color (unset bits are transparent).
void Adafruit_RA8875_drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {

  int16_t i, j, byteWidth = (w + 7) / 8;
  uint8_t byte = 0;

  for(j=0; j<h; j++) {
    for(i=0; i<w; i++) {
      if(i & 7) byte <<= 1;
      else      byte   = pgm_read_byte(bitmap + j * byteWidth + i / 8);
      if(byte & 0x80) Adafruit_RA8875_drawPixel(x+i, y+j, color);
    }
  }
}

// Draw a character
void Adafruit_RA8875_drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size) {
    int i, j;
  if(!gfxFont) { // 'Classic' built-in font

    if((x >= _width)            || // Clip right
       (y >= _height)           || // Clip bottom
       ((x + 6 * size - 1) < 0) || // Clip left
       ((y + 8 * size - 1) < 0))   // Clip top
      return;

    if(!cp437 && (c >= 176)) c++; // Handle 'classic' charset behavior

    for(i=0; i<6; i++ ) {
      uint8_t line;
      if(i < 5) line = pgm_read_byte(font+(c*5)+i);
      else      line = 0x0;
      for(j=0; j<8; j++, line >>= 1) {
        if(line & 0x1) {
          if(size == 1) Adafruit_RA8875_drawPixel(x+i, y+j, color);
          else          Adafruit_RA8875_fillRect(x+(i*size), y+(j*size), size, size, color);
        } else if(bg != color) {
          if(size == 1) Adafruit_RA8875_drawPixel(x+i, y+j, bg);
          else          Adafruit_RA8875_fillRect(x+i*size, y+j*size, size, size, bg);
        }
      }
    }

  } else { // Custom font

    // Character is assumed previously filtered by write() to eliminate
    // newlines, returns, non-printable characters, etc.  Calling drawChar()
    // directly with 'bad' characters of font may cause mayhem!

    c -= pgm_read_byte(&gfxFont->first);
    GFXglyph *glyph  = &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
    uint8_t  *bitmap = (uint8_t *)pgm_read_pointer(&gfxFont->bitmap);

    uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
    uint8_t  w  = pgm_read_byte(&glyph->width),
             h  = pgm_read_byte(&glyph->height),
             xa = pgm_read_byte(&glyph->xAdvance);
    int8_t   xo = pgm_read_byte(&glyph->xOffset),
             yo = pgm_read_byte(&glyph->yOffset);
    uint8_t  xx, yy, bits = 0, bit = 0;
    int16_t  xo16 = 0, yo16 = 0;

    if(size > 1) {
      xo16 = xo;
      yo16 = yo;
    }

    // Todo: Add character clipping here

    // NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
    // THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
    // has typically been used with the 'classic' font to overwrite old
    // screen contents with new data.  This ONLY works because the
    // characters are a uniform size; it's not a sensible thing to do with
    // proportionally-spaced fonts with glyphs of varying sizes (and that
    // may overlap).  To replace previously-drawn text when using a custom
    // font, use the getTextBounds() function to determine the smallest
    // rectangle encompassing a string, erase the area with fillRect(),
    // then draw new text.  This WILL infortunately 'blink' the text, but
    // is unavoidable.  Drawing 'background' pixels will NOT fix this,
    // only creates a new set of problems.  Have an idea to work around
    // this (a canvas object type for MCUs that can afford the RAM and
    // displays supporting setAddrWindow() and pushColors()), but haven't
    // implemented this yet.

    for(yy=0; yy<h; yy++) {
      for(xx=0; xx<w; xx++) {
        if(!(bit++ & 7)) {
          bits = pgm_read_byte(&bitmap[bo++]);
        }
        if(bits & 0x80) {
          if(size == 1) {
            Adafruit_RA8875_drawPixel(x+xo+xx, y+yo+yy, color);
          } else {
            Adafruit_RA8875_fillRect(x+(xo16+xx)*size, y+(yo16+yy)*size, size, size, color);
          }
        }
        bits <<= 1;
      }
    }

  } // End classic vs custom font
}

void Adafruit_RA8875_setCursor(int16_t x, int16_t y) {
  cursor_x = x;
  cursor_y = y;
}

void Adafruit_RA8875_setTextColor(uint16_t c, uint16_t b) {
  // For 'transparent' background, set the bg
  // to the same as fg instead of using a flag
  textcolor   = c;
  textbgcolor = b;
}

void Adafruit_RA8875_setTextSize(uint8_t s) {
  textsize = (s > 0) ? s : 1;
}

void Adafruit_RA8875_setTextWrap(int w) {
  wrap = w;
}

void Adafruit_RA8875_setRotation(uint8_t x) {
  rotation = (x & 3);
  switch(rotation) {
   case 0:
   case 2:
    _width  = WIDTH;
    _height = HEIGHT;
    break;
   case 1:
   case 3:
    _width  = HEIGHT;
    _height = WIDTH;
    break;
  }
}

void Adafruit_RA8875_setFont(const GFXfont *f) {
  if(f) {          // Font struct pointer passed in?
    if(!gfxFont) { // And no current font struct?
      // Switching from classic to new font behavior.
      // Move cursor pos down 6 pixels so it's on baseline.
      cursor_y += 6;
    }
  } else if(gfxFont) { // NULL passed.  Current font struct defined?
    // Switching from new to classic font behavior.
    // Move cursor pos up 6 pixels so it's at top-left of char.
    cursor_y -= 6;
  }
  gfxFont = (GFXfont *)f;
}

// Pass string and a cursor position, returns UL corner and W,H.
void Adafruit_RA8875_getTextBounds(char *str, int16_t x, int16_t y,
 int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
  uint8_t c; // Current character

  *x1 = x;
  *y1 = y;
  *w  = *h = 0;

  if(gfxFont) {

    GFXglyph *glyph;
    uint8_t   first = pgm_read_byte(&gfxFont->first),
              last  = pgm_read_byte(&gfxFont->last),
              gw, gh, xa;
    int8_t    xo, yo;
    int16_t   minx = _width, miny = _height, maxx = -1, maxy = -1,
              gx1, gy1, gx2, gy2, ts = (int16_t)textsize,
              ya = ts * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);

    while((c = *str++)) {
      if(c != '\n') { // Not a newline
        if(c != '\r') { // Not a carriage return, is normal char
          if((c >= first) && (c <= last)) { // Char present in current font
            c    -= first;
            glyph = &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
            gw    = pgm_read_byte(&glyph->width);
            gh    = pgm_read_byte(&glyph->height);
            xa    = pgm_read_byte(&glyph->xAdvance);
            xo    = pgm_read_byte(&glyph->xOffset);
            yo    = pgm_read_byte(&glyph->yOffset);
            if(wrap && ((x + (((int16_t)xo + gw) * ts)) >= _width)) {
              // Line wrap
              x  = 0;  // Reset x to 0
              y += ya; // Advance y by 1 line
            }
            gx1 = x   + xo * ts;
            gy1 = y   + yo * ts;
            gx2 = gx1 + gw * ts - 1;
            gy2 = gy1 + gh * ts - 1;
            if(gx1 < minx) minx = gx1;
            if(gy1 < miny) miny = gy1;
            if(gx2 > maxx) maxx = gx2;
            if(gy2 > maxy) maxy = gy2;
            x += xa * ts;
          }
        } // Carriage return = do nothing
      } else { // Newline
        x  = 0;  // Reset x
        y += ya; // Advance y by 1 line
      }
    }
    // End of string
    *x1 = minx;
    *y1 = miny;
    if(maxx >= minx) *w  = maxx - minx + 1;
    if(maxy >= miny) *h  = maxy - miny + 1;

  } else { // Default font

    uint16_t lineWidth = 0, maxWidth = 0; // Width of current, all lines

    while((c = *str++)) {
      if(c != '\n') { // Not a newline
        if(c != '\r') { // Not a carriage return, is normal char
          if(wrap && ((x + textsize * 6) >= _width)) {
            x  = 0;            // Reset x to 0
            y += textsize * 8; // Advance y by 1 line
            if(lineWidth > maxWidth) maxWidth = lineWidth; // Save widest line
            lineWidth  = textsize * 6; // First char on new line
          } else { // No line wrap, just keep incrementing X
            lineWidth += textsize * 6; // Includes interchar x gap
          }
        } // Carriage return = do nothing
      } else { // Newline
        x  = 0;            // Reset x to 0
        y += textsize * 8; // Advance y by 1 line
        if(lineWidth > maxWidth) maxWidth = lineWidth; // Save widest line
        lineWidth = 0;     // Reset lineWidth for new line
      }
    }
    // End of string
    if(lineWidth) y += textsize * 8; // Add height of last (or only) line
    *w = maxWidth - 1;               // Don't include last interchar x gap
    *h = y - *y1;

  } // End classic vs custom font
}

void Adafruit_RA8875_write(uint8_t c) {

  if(!gfxFont) { // 'Classic' built-in font

    if(c == '\n') {
      cursor_y += textsize*8;
      cursor_x  = 0;
    } else if(c == '\r') {
      // skip em
    } else {
      if(wrap && ((cursor_x + textsize * 6) >= _width)) { // Heading off edge?
        cursor_x  = 0;            // Reset x to zero
        cursor_y += textsize * 8; // Advance y one line
      }
      Adafruit_RA8875_drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
      cursor_x += textsize * 6;
    }

  } else { // Custom font

    if(c == '\n') {
      cursor_x  = 0;
      cursor_y += (int16_t)textsize *
                  (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
    } else if(c != '\r') {
      uint8_t first = pgm_read_byte(&gfxFont->first);
      if((c >= first) && (c <= (uint8_t)pgm_read_byte(&gfxFont->last))) {
        uint8_t   c2    = c - pgm_read_byte(&gfxFont->first);
        GFXglyph *glyph = &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c2]);
        uint8_t   w     = pgm_read_byte(&glyph->width),
                  h     = pgm_read_byte(&glyph->height);
        if((w > 0) && (h > 0)) { // Is there an associated bitmap?
          int16_t xo = (int8_t)pgm_read_byte(&glyph->xOffset); // sic
          if(wrap && ((cursor_x + textsize * (xo + w)) >= _width)) {
            // Drawing character would go off right edge; wrap to new line
            cursor_x  = 0;
            cursor_y += (int16_t)textsize *
                        (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
          }
          Adafruit_RA8875_drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
        }
        cursor_x += pgm_read_byte(&glyph->xAdvance) * (int16_t)textsize;
      }
    }

  }
}