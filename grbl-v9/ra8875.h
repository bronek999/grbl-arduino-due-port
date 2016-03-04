#ifndef RA8875_H_
#define RA8875_H_

#include <stdbool.h>


#define CAPACITIVE_TOUCH 0

#define ALINE_LEFT		0
#define ALINE_CENTER	1
#define ALINE_RIGHT		2

#define ScreenHeight 480
#define CalcHeight(medzera,pocet) (ScreenHeight / pocet - medzera)
#define CalcY(medzera,pocet,cislo) ((medzera / 2) + (CalcHeight(medzera,pocet) + medzera) * (cislo - 1))
#define CalcX(medzera,pocet,cislo) ((medzera / 2) + (CalcHeight(medzera,pocet) + medzera) * (cislo - 1))



// Sizes!
enum RA8875sizes { RA8875_480x272, RA8875_800x480 };

// Touch screen cal structs
typedef struct Point 
{
  int32_t x;
  int32_t y;
} tsPoint_t;

typedef struct //Matrix
{
  int32_t An,
          Bn,
          Cn,
          Dn,
          En,
          Fn,
          Divider ;
} tsMatrix_t;


#define CAPACITIVE_TOUCH	0	

typedef struct TRect_1 {
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;
}TRect;

//	Struktura ma 15B size
typedef struct TGraphics_1{
	TRect Rect;
	unsigned char handle;
	unsigned char type;
	unsigned char value;
	unsigned int BackColor;
	unsigned int Color;
	struct TGraphics_1 *next;
	char *text;	//	Toto pole moze mat roznu dlzku
	char radius;
} TGraphics ;

typedef struct
{ 
   int32_t    x;
   int32_t    y;
}POINT;


#define  MATRIX_Type  int32_t

typedef struct
{
							/* This arrangement of values facilitates 
							 *  calculations within getDisplayPoint() 
							 */
   MATRIX_Type    An;     /* A = An/Divider */
   MATRIX_Type    Bn;     /* B = Bn/Divider */
   MATRIX_Type    Cn;     /* C = Cn/Divider */
   MATRIX_Type    Dn;     /* D = Dn/Divider */
   MATRIX_Type    En;     /* E = En/Divider */
   MATRIX_Type    Fn;     /* F = Fn/Divider */
   MATRIX_Type    Divider ;
}TOUCH_MATRIX;

 uint16_t tft_width, tft_height;
 bool tft_is_text_mode;
 uint8_t tft_textScale;
 uint8_t tft_cursor_type;
extern volatile int Action,Action1;
extern bool needtouch;
 
#define lcd_spi_cs_low() while(!(PIOD->PIO_PDSR & (PIO_PD8)));PIOD->PIO_CODR = PIO_PD8;
#define lcd_spi_cs_high() PIOD->PIO_SODR = PIO_PD8;


extern int setCalibrationMatrix( POINT * display,POINT * screen,TOUCH_MATRIX * matrix) ;
extern int getDisplayPoint( POINT * display,POINT * screen,TOUCH_MATRIX * matrix ) ;


uint8_t  tft_readReg(uint8_t reg);
void tft_initialize(void);
void  tft_writeCommand(uint8_t d);
void  tft_writeData(uint8_t d);
void  tft_writeReg(uint8_t reg, uint8_t val);
uint8_t  tft_readReg(uint8_t reg);
void tft_textEnlarge(uint8_t scale);
uint8_t  tft_readData(void);
//void tft_rectHelper(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, bool filled);
void tft_rectHelperRound(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, bool filled, uint8_t radius);
void tft_circleHelper(int16_t x0, int16_t y0, int16_t r, uint16_t color, bool filled);
void tft_triangleHelper(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color, bool filled);
void tft_ellipseHelper(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color, bool filled);
void tft_curveHelper(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint8_t curvePart, uint16_t color, bool filled);
void tft_graphicCursorEnable(void);
void tft_add_object(unsigned int x, unsigned int y, unsigned int width1, unsigned int height1, unsigned char handle, unsigned char type, unsigned char value,unsigned int BackColor,unsigned int Color, const char *text, char radius);
void tft_frame_draw(unsigned char handle);
void tft_button(unsigned int x,unsigned int y,unsigned int width1,unsigned int height1,const char *text, unsigned char handle, uint16_t color,uint8_t radius);
void tft_GetTouch();
void beep(int ms);
extern void tft_init();
void tft_displayOn(bool on);
void tft_GPIOX(bool on);
void tft_PWM1config(bool on, uint8_t clock);
void tft_PWM1out(uint8_t p);
void tft_graphicsMode(void);
void tft_fillScreen(uint16_t color);
void tft_textMode(void) ;
void tft_textColor(uint16_t foreColor, uint16_t bgColor);
extern void beep(int ms);
extern void tft_textWrite(uint16_t left,uint16_t top,uint16_t right, uint16_t bottom, char* buffer, uint8_t align);
extern void tft_draw_press_unpress(void);
void tft_InitTimer();
extern void tft_calibrate_touch(void) ;
extern void tft_calibrate_load(void);
void tft_touchEnable(bool on);
extern void tft_delete_all_objects(void);
extern int tft_GetInt(char *Text,int defval, int min, int max);

#define RGB(r,g,b)	(((b / 8)  << 0) | ((g / 4) << 5) | ((r / 8) << 11))

// Colors (RGB565)
#define	RA8875_BLACK            0x0000
#define	RA8875_BLUE             0x001F
#define	RA8875_RED              0xF800
#define	RA8875_GREEN            0x07E0
#define RA8875_CYAN             0x07FF
#define RA8875_MAGENTA          0xF81F
#define RA8875_YELLOW           0xFFE0
#define RA8875_WHITE            0xFFFF
#define RA8875_LIME             RGB(0xCC, 0xFF, 0x00)
#define RA885_LIGHTGRAY			RGB(210,210,210)

// Command/Data pins for SPI
#define RA8875_DATAWRITE        0x00
#define RA8875_DATAREAD         0x40
#define RA8875_CMDWRITE         0x80
#define RA8875_CMDREAD          0xC0

// Registers & bits
#define RA8875_PWRR             0x01
#define RA8875_PWRR_DISPON      0x80
#define RA8875_PWRR_DISPOFF     0x00
#define RA8875_PWRR_SLEEP       0x02
#define RA8875_PWRR_NORMAL      0x00
#define RA8875_PWRR_SOFTRESET   0x01

#define RA8875_MRWC             0x02

#define RA8875_GPIOX            0xC7

#define RA8875_PLLC1            0x88
#define RA8875_PLLC1_PLLDIV2    0x80
#define RA8875_PLLC1_PLLDIV1    0x00

#define RA8875_PLLC2            0x89
#define RA8875_PLLC2_DIV1       0x00
#define RA8875_PLLC2_DIV2       0x01
#define RA8875_PLLC2_DIV4       0x02
#define RA8875_PLLC2_DIV8       0x03
#define RA8875_PLLC2_DIV16      0x04
#define RA8875_PLLC2_DIV32      0x05
#define RA8875_PLLC2_DIV64      0x06
#define RA8875_PLLC2_DIV128     0x07

#define RA8875_SYSR             0x10
#define RA8875_SYSR_8BPP        0x00
#define RA8875_SYSR_16BPP       0x0C
#define RA8875_SYSR_MCU8        0x00
#define RA8875_SYSR_MCU16       0x03

#define RA8875_PCSR             0x04
#define RA8875_PCSR_PDATR       0x00
#define RA8875_PCSR_PDATL       0x80
#define RA8875_PCSR_CLK         0x00
#define RA8875_PCSR_2CLK        0x01
#define RA8875_PCSR_4CLK        0x02
#define RA8875_PCSR_8CLK        0x03

#define RA8875_HDWR             0x14

#define RA8875_HNDFTR           0x15
#define RA8875_HNDFTR_DE_HIGH   0x00
#define RA8875_HNDFTR_DE_LOW    0x80

#define RA8875_HNDR             0x16
#define RA8875_HSTR             0x17
#define RA8875_HPWR             0x18
#define RA8875_HPWR_LOW         0x00
#define RA8875_HPWR_HIGH        0x80

#define RA8875_VDHR0            0x19
#define RA8875_VDHR1            0x1A
#define RA8875_VNDR0            0x1B
#define RA8875_VNDR1            0x1C
#define RA8875_VSTR0            0x1D
#define RA8875_VSTR1            0x1E
#define RA8875_VPWR             0x1F
#define RA8875_VPWR_LOW         0x00
#define RA8875_VPWR_HIGH        0x80

#define RA8875_HSAW0            0x30
#define RA8875_HSAW1            0x31
#define RA8875_VSAW0            0x32
#define RA8875_VSAW1            0x33

#define RA8875_HEAW0            0x34
#define RA8875_HEAW1            0x35
#define RA8875_VEAW0            0x36
#define RA8875_VEAW1            0x37

#define RA8875_MCLR             0x8E
#define RA8875_MCLR_START       0x80
#define RA8875_MCLR_STOP        0x00
#define RA8875_MCLR_READSTATUS  0x80
#define RA8875_MCLR_FULL        0x00
#define RA8875_MCLR_ACTIVE      0x40

#define RA8875_DCR                    0x90
#define RA8875_DCR_LINESQUTRI_START   0x80
#define RA8875_DCR_LINESQUTRI_STOP    0x00
#define RA8875_DCR_LINESQUTRI_STATUS  0x80
#define RA8875_DCR_CIRCLE_START       0x40
#define RA8875_DCR_CIRCLE_STATUS      0x40
#define RA8875_DCR_CIRCLE_STOP        0x00
#define RA8875_DCR_FILL               0x20
#define RA8875_DCR_NOFILL             0x00
#define RA8875_DCR_DRAWLINE           0x00
#define RA8875_DCR_DRAWTRIANGLE       0x01
#define RA8875_DCR_DRAWSQUARE         0x10


#define RA8875_ELLIPSE                0xA0
#define RA8875_ELLIPSE_STATUS         0x80

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

#define RA8875_TPCR0                  0x70
#define RA8875_TPCR0_ENABLE           0x80
#define RA8875_TPCR0_DISABLE          0x00
#define RA8875_TPCR0_WAIT_512CLK      0x00
#define RA8875_TPCR0_WAIT_1024CLK     0x10
#define RA8875_TPCR0_WAIT_2048CLK     0x20
#define RA8875_TPCR0_WAIT_4096CLK     0x30
#define RA8875_TPCR0_WAIT_8192CLK     0x40
#define RA8875_TPCR0_WAIT_16384CLK    0x50
#define RA8875_TPCR0_WAIT_32768CLK    0x60
#define RA8875_TPCR0_WAIT_65536CLK    0x70
#define RA8875_TPCR0_WAKEENABLE       0x08
#define RA8875_TPCR0_WAKEDISABLE      0x00
#define RA8875_TPCR0_ADCCLK_DIV1      0x00
#define RA8875_TPCR0_ADCCLK_DIV2      0x01
#define RA8875_TPCR0_ADCCLK_DIV4      0x02
#define RA8875_TPCR0_ADCCLK_DIV8      0x03
#define RA8875_TPCR0_ADCCLK_DIV16     0x04
#define RA8875_TPCR0_ADCCLK_DIV32     0x05
#define RA8875_TPCR0_ADCCLK_DIV64     0x06
#define RA8875_TPCR0_ADCCLK_DIV128    0x07

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


#endif /* RA8875_H_ */