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

#include <cytypes.h>
#include "gfxfont.h"

#ifndef _ADAFRUIT_RA8875_H
#define _ADAFRUIT_RA8875_H

// Touch screen cal structs
typedef struct Point {
    int32_t x;
    int32_t y;
} tsPoint_t;

typedef struct /*Matrix */ {
    int32_t An,
            Bn,
            Cn,
            Dn,
            En,
            Fn,
            Divider;
} tsMatrix_t;

/****************** Start of "Class" definitions ******************/
/* "Private" class definitions */
/* GFX Helper Functions */
void Adafruit_RA8875_circleHelper(int16_t x0, int16_t y0, int16_t r, uint16_t color, int filled);
void Adafruit_RA8875_rectHelper  (int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, int filled);
void Adafruit_RA8875_triangleHelper(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color, int filled);
void Adafruit_RA8875_ellipseHelper(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color, int filled);
void Adafruit_RA8875_curveHelper(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint8_t curvePart, uint16_t color, int filled);

/* "Public" class definitions */
int Adafruit_RA8875_begin();
void Adafruit_RA8875_softReset(void);
void Adafruit_RA8875_displayOn(int on);
void Adafruit_RA8875_sleep(int sleep);
void Adafruit_RA8875_setOrientation(uint8 mode);

/* Text functions */
void Adafruit_RA8875_textMode(void);
void Adafruit_RA8875_textSetCursor(uint16_t x, uint16_t y);
void Adafruit_RA8875_textColor(uint16_t foreColor, uint16_t bgColor);
void Adafruit_RA8875_textTransparent(uint16_t foreColor);
void Adafruit_RA8875_textEnlarge(uint8_t scale);
void Adafruit_RA8875_textWrite(const char* buffer, uint16_t len);

/* Graphics functions */
void Adafruit_RA8875_graphicsMode(void);
void Adafruit_RA8875_setXY(uint16_t x, uint16_t y);
void Adafruit_RA8875_pushPixels(uint32_t num, uint16_t p);
void Adafruit_RA8875_fillRectEmpty(void);

/* Adafruit_GFX functions */
void Adafruit_RA8875_drawPixel(int16_t x, int16_t y, uint16_t color);
void Adafruit_RA8875_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void Adafruit_RA8875_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);

/* HW accelerated wrapper functions (override Adafruit_GFX prototypes) */
void Adafruit_RA8875_fillScreen(uint16_t color);
void Adafruit_RA8875_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void Adafruit_RA8875_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void Adafruit_RA8875_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void Adafruit_RA8875_drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void Adafruit_RA8875_fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void Adafruit_RA8875_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void Adafruit_RA8875_fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void Adafruit_RA8875_drawEllipse(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color);
void Adafruit_RA8875_fillEllipse(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color);
void Adafruit_RA8875_drawCurve(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint8_t curvePart, uint16_t color);
void Adafruit_RA8875_fillCurve(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint8_t curvePart, uint16_t color);

/* Backlight */
void Adafruit_RA8875_GPIOX(int on);
void Adafruit_RA8875_PWM1config(int on, uint8_t clock);
void Adafruit_RA8875_PWM2config(int on, uint8_t clock);
void Adafruit_RA8875_PWM1out(uint8_t p);
void Adafruit_RA8875_PWM2out(uint8_t p);

/* Touch screen */
void Adafruit_RA8875_touchEnable(int on);
int  Adafruit_RA8875_touched(void);
int  Adafruit_RA8875_touchRead(uint16_t *x, uint16_t *y);

/* Low level access */
void    Adafruit_RA8875_writeReg(uint8_t reg, uint8_t val);
uint8_t Adafruit_RA8875_readReg(uint8_t reg);
void    Adafruit_RA8875_writeData(uint8_t d);
uint8_t Adafruit_RA8875_readData(void);
void    Adafruit_RA8875_writeCommand(uint8_t d);
uint8_t Adafruit_RA8875_readStatus(void);
int     Adafruit_RA8875_waitPoll(uint8_t r, uint8_t f);

void Adafruit_RA8875_PLLinit(void);
void Adafruit_RA8875_initialize(void);

/* Functions inherited by GFX */
void Adafruit_RA8875_drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
void Adafruit_RA8875_drawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);
void Adafruit_RA8875_fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
void Adafruit_RA8875_fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color);
void Adafruit_RA8875_drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
void Adafruit_RA8875_drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
void Adafruit_RA8875_setCursor(int16_t x, int16_t y);
void Adafruit_RA8875_setTextColor(uint16_t c, uint16_t b);
void Adafruit_RA8875_setTextSize(uint8_t s);
void Adafruit_RA8875_setTextWrap(int w);
void Adafruit_RA8875_setRotation(uint8_t x);
void Adafruit_RA8875_setFont(const GFXfont *f);
void Adafruit_RA8875_getTextBounds(char *str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);
void Adafruit_RA8875_write(uint8_t c);
/****************** End of "Class" definitions ******************/

// Colors (RGB565)
#define	RA8875_BLACK           0x0000
#define	RA8875_BLUE            0x001F
#define	RA8875_RED             0xF800
#define	RA8875_GREEN           0x07E0
#define RA8875_CYAN            0x07FF
#define RA8875_MAGENTA         0xF81F
#define RA8875_YELLOW          0xFFE0  
#define RA8875_WHITE           0xFFFF

// Command/Data pins for SPI
#define RA8875_DATAWRITE       0x00
#define RA8875_DATAREAD        0x40
#define RA8875_CMDWRITE        0x80
#define RA8875_CMDREAD         0xC0

// Registers & bits
#define RA8875_PWRR            0x01
#define RA8875_PWRR_DISPON     0x80
#define RA8875_PWRR_DISPOFF    0x00
#define RA8875_PWRR_SLEEP      0x02
#define RA8875_PWRR_NORMAL     0x00
#define RA8875_PWRR_SOFTRESET  0x01

#define RA8875_MRWC            0x02

#define RA8875_GPIOX           0xC7

#define RA8875_PLLC1           0x88
#define RA8875_PLLC1_PLLDIV2   0x80
#define RA8875_PLLC1_PLLDIV1   0x00

#define RA8875_PLLC2           0x89
#define RA8875_PLLC2_DIV1      0x00
#define RA8875_PLLC2_DIV2      0x01
#define RA8875_PLLC2_DIV4      0x02
#define RA8875_PLLC2_DIV8      0x03
#define RA8875_PLLC2_DIV16     0x04
#define RA8875_PLLC2_DIV32     0x05
#define RA8875_PLLC2_DIV64     0x06
#define RA8875_PLLC2_DIV128    0x07

#define RA8875_SYSR            0x10
#define RA8875_SYSR_8BPP       0x00
#define RA8875_SYSR_16BPP      0x0C
#define RA8875_SYSR_MCU8       0x00
#define RA8875_SYSR_MCU16      0x03

#define RA8875_PCSR            0x04
#define RA8875_PCSR_PDATR      0x00
#define RA8875_PCSR_PDATL      0x80
#define RA8875_PCSR_CLK        0x00
#define RA8875_PCSR_2CLK       0x01
#define RA8875_PCSR_4CLK       0x02
#define RA8875_PCSR_8CLK       0x03

#define RA8875_HDWR            0x14

#define RA8875_HNDFTR          0x15
#define RA8875_HNDFTR_DE_HIGH  0x00
#define RA8875_HNDFTR_DE_LOW   0x80

#define RA8875_HNDR            0x16
#define RA8875_HSTR            0x17
#define RA8875_HPWR            0x18
#define RA8875_HPWR_LOW        0x00
#define RA8875_HPWR_HIGH       0x80

#define RA8875_VDHR0           0x19
#define RA8875_VDHR1           0x1A
#define RA8875_VNDR0           0x1B
#define RA8875_VNDR1           0x1C
#define RA8875_VSTR0           0x1D
#define RA8875_VSTR1           0x1E
#define RA8875_VPWR            0x1F
#define RA8875_VPWR_LOW        0x00
#define RA8875_VPWR_HIGH       0x80

#define RA8875_HSAW0           0x30
#define RA8875_HSAW1           0x31
#define RA8875_VSAW0           0x32
#define RA8875_VSAW1           0x33

#define RA8875_HEAW0           0x34
#define RA8875_HEAW1           0x35
#define RA8875_VEAW0           0x36
#define RA8875_VEAW1           0x37

#define RA8875_MCLR            0x8E
#define RA8875_MCLR_START      0x80
#define RA8875_MCLR_STOP       0x00
#define RA8875_MCLR_READSTATUS 0x80
#define RA8875_MCLR_FULL       0x00
#define RA8875_MCLR_ACTIVE     0x40

#define RA8875_DCR                   0x90
#define RA8875_DCR_LINESQUTRI_START  0x80
#define RA8875_DCR_LINESQUTRI_STOP   0x00
#define RA8875_DCR_LINESQUTRI_STATUS 0x80
#define RA8875_DCR_CIRCLE_START      0x40
#define RA8875_DCR_CIRCLE_STATUS     0x40
#define RA8875_DCR_CIRCLE_STOP       0x00
#define RA8875_DCR_FILL              0x20
#define RA8875_DCR_NOFILL            0x00
#define RA8875_DCR_DRAWLINE          0x00
#define RA8875_DCR_DRAWTRIANGLE      0x01
#define RA8875_DCR_DRAWSQUARE        0x10

#define RA8875_ELLIPSE               0xA0
#define RA8875_ELLIPSE_STATUS        0x80

#define RA8875_MWCR0            0x40
#define RA8875_MWCR0_GFXMODE    0x00
#define RA8875_MWCR0_TXTMODE    0x80

#define RA8875_CURH0            0x46
#define RA8875_CURH1            0x47
#define RA8875_CURV0            0x48
#define RA8875_CURV1            0x49

#define RA8875_P1CR             0x8A
#define RA8875_P1CR_ENABLE      0x80
#define RA8875_P1CR_DISABLE     0x00
#define RA8875_P1CR_CLKOUT      0x10
#define RA8875_P1CR_PWMOUT      0x00

#define RA8875_P1DCR            0x8B

#define RA8875_P2CR             0x8C
#define RA8875_P2CR_ENABLE      0x80
#define RA8875_P2CR_DISABLE     0x00
#define RA8875_P2CR_CLKOUT      0x10
#define RA8875_P2CR_PWMOUT      0x00

#define RA8875_P2DCR            0x8D

#define RA8875_PWM_CLK_DIV1     0x00
#define RA8875_PWM_CLK_DIV2     0x01
#define RA8875_PWM_CLK_DIV4     0x02
#define RA8875_PWM_CLK_DIV8     0x03
#define RA8875_PWM_CLK_DIV16    0x04
#define RA8875_PWM_CLK_DIV32    0x05
#define RA8875_PWM_CLK_DIV64    0x06
#define RA8875_PWM_CLK_DIV128   0x07
#define RA8875_PWM_CLK_DIV256   0x08
#define RA8875_PWM_CLK_DIV512   0x09
#define RA8875_PWM_CLK_DIV1024  0x0A
#define RA8875_PWM_CLK_DIV2048  0x0B
#define RA8875_PWM_CLK_DIV4096  0x0C
#define RA8875_PWM_CLK_DIV8192  0x0D
#define RA8875_PWM_CLK_DIV16384 0x0E
#define RA8875_PWM_CLK_DIV32768 0x0F

#define RA8875_TPCR0               0x70
#define RA8875_TPCR0_ENABLE        0x80
#define RA8875_TPCR0_DISABLE       0x00
#define RA8875_TPCR0_WAIT_512CLK   0x00
#define RA8875_TPCR0_WAIT_1024CLK  0x10
#define RA8875_TPCR0_WAIT_2048CLK  0x20
#define RA8875_TPCR0_WAIT_4096CLK  0x30
#define RA8875_TPCR0_WAIT_8192CLK  0x40
#define RA8875_TPCR0_WAIT_16384CLK 0x50
#define RA8875_TPCR0_WAIT_32768CLK 0x60
#define RA8875_TPCR0_WAIT_65536CLK 0x70
#define RA8875_TPCR0_WAKEENABLE    0x08
#define RA8875_TPCR0_WAKEDISABLE   0x00
#define RA8875_TPCR0_ADCCLK_DIV1   0x00
#define RA8875_TPCR0_ADCCLK_DIV2   0x01
#define RA8875_TPCR0_ADCCLK_DIV4   0x02
#define RA8875_TPCR0_ADCCLK_DIV8   0x03
#define RA8875_TPCR0_ADCCLK_DIV16  0x04
#define RA8875_TPCR0_ADCCLK_DIV32  0x05
#define RA8875_TPCR0_ADCCLK_DIV64  0x06
#define RA8875_TPCR0_ADCCLK_DIV128 0x07

#define RA8875_TPCR1            0x71
#define RA8875_TPCR1_AUTO       0x00
#define RA8875_TPCR1_MANUAL     0x40
#define RA8875_TPCR1_VREFINT    0x00
#define RA8875_TPCR1_VREFEXT    0x20
#define RA8875_TPCR1_DEBOUNCE   0x04
#define RA8875_TPCR1_NODEBOUNCE 0x00
#define RA8875_TPCR1_IDLE       0x00
#define RA8875_TPCR1_WAIT       0x01
#define RA8875_TPCR1_LATCHX     0x02
#define RA8875_TPCR1_LATCHY     0x03

#define RA8875_TPXH             0x72
#define RA8875_TPYH             0x73
#define RA8875_TPXYL            0x74

#define RA8875_INTC1            0xF0
#define RA8875_INTC1_KEY        0x10
#define RA8875_INTC1_DMA        0x08
#define RA8875_INTC1_TP         0x04
#define RA8875_INTC1_BTE        0x02

#define RA8875_INTC2            0xF1
#define RA8875_INTC2_KEY        0x10
#define RA8875_INTC2_DMA        0x08
#define RA8875_INTC2_TP         0x04
#define RA8875_INTC2_BTE        0x02

#endif