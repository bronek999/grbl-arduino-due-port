#include "sam.h"
#include "ra8875.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "flash_storage.h"

TGraphics *Gall,*Gcurrent;
volatile int TouchXPos,TouchYPos,LastTouchXPos,LastTouchYPos;
volatile bool IsTouched=false;
volatile bool unpressed=false;
volatile bool dotyk;
volatile int Action,Action1;
bool needtouch = false;
bool Calibrating = false;
TOUCH_MATRIX TouchCalibration;


unsigned char Slovak[36]= {'·','‰','Ë','Ô','È','Ì','Â','æ','Ú','Û','Ù','‡','ú','ö','ù','˙','˝','û','¡','ƒ','»','œ','…','Õ','≈','º','“','”','‘','¿','å','ä','ç','⁄','›','é'};
unsigned char CP852[36]=  {225,228,232,239,233,237,229,181,242,243,244,224,182,185,187,250,253,190,193,196,200,207,201,205,197,165,210,211,212,192,166,169,171,218,221,174};
void SlovakTo852(char *text)
{
	int i;
	while (*text != 0)
	{
		for(i = 0; i< 36;i++)
		if(*text == Slovak[i])
		{
			*text = CP852[i];
			break;
		}
		text++ ;
	}
}
unsigned int PosR(char znak,char *text)	//	Najde poslednu poziciu znaku v texte
{
	char lp=0;
	int c=0;
	while(text[c])
	{
		if(text[c] == znak)
		lp = c;
		c++;
	}
	return lp;
}

unsigned char LCD_SPIWrite(uint8_t val)
{
	//Wait for previous transfer to complete
	while ((SPI0->SPI_SR & SPI_SR_TXEMPTY) == 0);
	
	//load the Transmit Data Register with the value to transmit
	SPI0->SPI_TDR = val;
	
	//Wait for data to be transferred to serializer
	while ((SPI0->SPI_SR & SPI_SR_TDRE) == 0);
	
	while ( (SPI0->SPI_SR & SPI_SR_RDRF) == 0 ) ;

	return SPI0->SPI_RDR & 0x00FF ;
}

void tft_init()
{ 
	  tft_width = 800;
	  tft_height = 480;
	  tft_is_text_mode = false;
	  PMC->PMC_PCER0 |= 1 << ID_PIOA;
	  PMC->PMC_PCER0 |= 1 << ID_PIOB;
	  PMC->PMC_PCER0 |= 1 << ID_PIOD;
	  
	  PIOA->PIO_PDR |= PIO_PA25;		//MISO
	  PIOA->PIO_ODR |= PIO_PA25;		//Input
	  
	  PIOA->PIO_PDR |= PIO_PA26;		//MOSI
	  PIOA->PIO_OER |= PIO_PA26;		//MOSI	Output
	  PIOA->PIO_ABSR &= ~PIO_PA26;		//Peripheral A
	  
	  PIOA->PIO_PDR |= PIO_PA27;		//SPCK
	  PIOA->PIO_OER |= PIO_PA27;		//SPCK	Output
	  PIOA->PIO_ABSR &= ~PIO_PA27;		//Peripheral A
	  
	  PIOD->PIO_OER |= PIO_PD8;			//CS pre displej
	  //	PIOA->PIO_ABSR &= ~PIO_PA29;	//Peripheral B
	  PIOD->PIO_PUER |= PIO_PD8;		//pull-up
	  lcd_spi_cs_high();				//


	  //Enable clock for the SPI0 peripheral
	  PMC->PMC_PCER0 |= 1 << ID_SPI0;
	  //Disable the SPI0 peripheral so we can configure it.
	  SPI0->SPI_CR = SPI_CR_SPIDIS;
	  //Set as Master, Fixed Peripheral Select, Mode Fault Detection disabled and
	  //	Peripheral Chip Select is PCS = xxx0 NPCS[3:0] = 1110
	  SPI0->SPI_MR = SPI_MR_MSTR | SPI_MR_MODFDIS | 0x000e0000;
	  //SPCK baudrate = MCK / SCBR = 84MHz / 128 = 656250Hz
	  SPI0->SPI_CSR[0] |= 0x0000FF00  | 1 << 1;
	  //Enable the SPI0 unit
	  SPI0->SPI_CR = SPI_CR_SPIEN;
	  delay_ms(10);
	  volatile unsigned char x;
	  x = tft_readReg(0);
	  if (x != 0x75) {
		  return ;
	  }
	tft_initialize();
  	tft_displayOn(true);
	tft_GPIOX(true);      // Enable TFT - display enable tied to GPIOX
	tft_PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
	tft_PWM1out(255);
	  	
	tft_graphicsMode();                 // go back to graphics mode
	tft_fillScreen(RA8875_LIME);
	tft_textMode();
	tft_textEnlarge(0);
	tft_textColor(RA8875_WHITE,RA8875_BLACK);
	tft_touchEnable(true);
	tft_InitTimer();
}


/************************* Initialization *********************************/

/**************************************************************************/
/*!
      Performs a SW-based reset of the RA8875
*/
/**************************************************************************/
void tft_softReset(void) {
  tft_writeCommand(RA8875_PWRR);
  tft_writeData(RA8875_PWRR_SOFTRESET);
  tft_writeData(RA8875_PWRR_NORMAL);
  delay_ms(1);
}

/**************************************************************************/
/*!
      Initialise the PLL
*/
/**************************************************************************/
void tft_PLLinit(void) {
    tft_writeReg(RA8875_PLLC1, RA8875_PLLC1_PLLDIV1 + 10);
    delay_ms(1);
    tft_writeReg(RA8875_PLLC2, RA8875_PLLC2_DIV4);
    delay_ms(1);
}

/**************************************************************************/
/*!
      Initialises the driver IC (clock setup, etc.)
*/
/**************************************************************************/
void tft_initialize(void) {
  tft_PLLinit();
  tft_writeReg(RA8875_SYSR, RA8875_SYSR_16BPP | RA8875_SYSR_MCU8);

  /* Timing values */
  uint8_t pixclk;
  uint8_t hsync_start;
  uint8_t hsync_pw;
  uint8_t hsync_finetune;
  uint8_t hsync_nondisp;
  uint8_t vsync_pw; 
  uint16_t vsync_nondisp;
  uint16_t vsync_start;

  /* Set the correct values for the display being used  for 800x480*/  
    pixclk          = RA8875_PCSR_PDATL | RA8875_PCSR_2CLK;
    hsync_nondisp   = 26;
    hsync_start     = 32;
    hsync_pw        = 96;
    hsync_finetune  = 0;
    vsync_nondisp   = 32;
    vsync_start     = 23;
    vsync_pw        = 2;

  tft_writeReg(RA8875_PCSR, pixclk);
  delay_ms(1);
  
  /* Horizontal settings registers */
  tft_writeReg(RA8875_HDWR, (tft_width / 8) - 1);                          // H width: (HDWR + 1) * 8 = 480
  tft_writeReg(RA8875_HNDFTR, RA8875_HNDFTR_DE_HIGH + hsync_finetune);
  tft_writeReg(RA8875_HNDR, (hsync_nondisp - hsync_finetune - 2)/8);    // H non-display: HNDR * 8 + HNDFTR + 2 = 10
  tft_writeReg(RA8875_HSTR, hsync_start/8 - 1);                         // Hsync start: (HSTR + 1)*8 
  tft_writeReg(RA8875_HPWR, RA8875_HPWR_LOW + (hsync_pw/8 - 1));        // HSync pulse width = (HPWR+1) * 8
  
  /* Vertical settings registers */
  tft_writeReg(RA8875_VDHR0, (uint16_t)(tft_height - 1) & 0xFF);
  tft_writeReg(RA8875_VDHR1, (uint16_t)(tft_height - 1) >> 8);
  tft_writeReg(RA8875_VNDR0, vsync_nondisp-1);                          // V non-display period = VNDR + 1
  tft_writeReg(RA8875_VNDR1, vsync_nondisp >> 8);
  tft_writeReg(RA8875_VSTR0, vsync_start-1);                            // Vsync start position = VSTR + 1
  tft_writeReg(RA8875_VSTR1, vsync_start >> 8);
  tft_writeReg(RA8875_VPWR, RA8875_VPWR_LOW + vsync_pw - 1);            // Vsync pulse width = VPWR + 1
  
  /* Set active window X */
  tft_writeReg(RA8875_HSAW0, 0);                                        // horizontal start point
  tft_writeReg(RA8875_HSAW1, 0);
  tft_writeReg(RA8875_HEAW0, (uint16_t)(tft_width - 1) & 0xFF);            // horizontal end point
  tft_writeReg(RA8875_HEAW1, (uint16_t)(tft_width - 1) >> 8);
  
  /* Set active window Y */
  tft_writeReg(RA8875_VSAW0, 0);                                        // vertical start point
  tft_writeReg(RA8875_VSAW1, 0);  
  tft_writeReg(RA8875_VEAW0, (uint16_t)(tft_height - 1) & 0xFF);           // horizontal end point
  tft_writeReg(RA8875_VEAW1, (uint16_t)(tft_height - 1) >> 8);
  

  /* ToDo: Setup touch panel? */
  
  /* Clear the entire window */
  tft_writeReg(RA8875_MCLR, RA8875_MCLR_START | RA8875_MCLR_FULL);
  delay_ms(10); 
  tft_textEnlarge(0);
}

/**************************************************************************/
/*!
      Sets the display in text mode (as opposed to graphics mode)
*/
/**************************************************************************/
void tft_textMode(void) 
{
  /* Set text mode */
  tft_writeCommand(RA8875_MWCR0);
  uint8_t temp = tft_readData();
  temp |= RA8875_MWCR0_TXTMODE; // Set bit 7
  tft_writeData(temp);
  
  /* Select the internal (ROM) font */
  tft_writeCommand(0x21);
  temp = tft_readData();
  temp &= ~((1<<7) | (1<<5)) | (1<<1); // Clear bits 7 and 5 and 1
  temp |= (1 << 0);	// ISO/IEC 8859-2
  tft_writeData(temp);
  tft_is_text_mode = true;
}

/**************************************************************************/
/*!
      Sets the display in text mode (as opposed to graphics mode)
      
      @args x[in] The x position of the cursor (in pixels, 0..1023)
      @args y[in] The y position of the cursor (in pixels, 0..511)
*/
/**************************************************************************/
void tft_textSetCursor(uint16_t x, uint16_t y) 
{
  /* Set cursor location */
  tft_writeCommand(0x2A);
  tft_writeData(x & 0xFF);
  tft_writeCommand(0x2B);
  tft_writeData(x >> 8);
  tft_writeCommand(0x2C);
  tft_writeData(y & 0xFF);
  tft_writeCommand(0x2D);
  tft_writeData(y >> 8);
}

/**************************************************************************/
/*!
      Sets the fore and background color when rendering text
      
      @args foreColor[in] The RGB565 color to use when rendering the text
      @args bgColor[in]   The RGB565 colot to use for the background
*/
/**************************************************************************/
void tft_textColor(uint16_t foreColor, uint16_t bgColor)
{
  /* Set Fore Color */
  tft_writeCommand(0x63);
  tft_writeData((foreColor & 0xf800) >> 11);
  tft_writeCommand(0x64);
  tft_writeData((foreColor & 0x07e0) >> 5);
  tft_writeCommand(0x65);
  tft_writeData((foreColor & 0x001f));
  
  /* Set Background Color */
  tft_writeCommand(0x60);
  tft_writeData((bgColor & 0xf800) >> 11);
  tft_writeCommand(0x61);
  tft_writeData((bgColor & 0x07e0) >> 5);
  tft_writeCommand(0x62);
  tft_writeData((bgColor & 0x001f));
  
  /* Clear transparency flag */
  tft_writeCommand(0x22);
  uint8_t temp = tft_readData();
  temp &= ~(1<<6); // Clear bit 6
  tft_writeData(temp);
}

/**************************************************************************/
/*!
      Sets the fore color when rendering text with a transparent bg
      
      @args foreColor[in] The RGB565 color to use when rendering the text
*/
/**************************************************************************/
void tft_textTransparent(uint16_t foreColor)
{
  /* Set Fore Color */
  tft_writeCommand(0x63);
  tft_writeData((foreColor & 0xf800) >> 11);
  tft_writeCommand(0x64);
  tft_writeData((foreColor & 0x07e0) >> 5);
  tft_writeCommand(0x65);
  tft_writeData((foreColor & 0x001f));

  /* Set transparency flag */
  tft_writeCommand(0x22);
  uint8_t temp = tft_readData();
  temp |= (1<<6); // Set bit 6
  tft_writeData(temp);  
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
void tft_textEnlarge(uint8_t scale)
{
  if (scale > 3) scale = 3;

  /* Set font size flags */
  tft_writeCommand(0x22);
  uint8_t temp = tft_readData();
  temp &= ~(0xF); // Clears bits 0..3
  temp |= scale << 2;
  temp |= scale;
  tft_writeData(temp);  

  tft_textScale = scale;
}

/**************************************************************************/
/*!
      Renders some text on the screen when in text mode
      
      @args buffer[in]    The buffer containing the characters to render
      @args len[in]       The size of the buffer in bytes
*/
/**************************************************************************/
void tft_textWriteRaw(const char* buffer, uint16_t len) 
{
  if (len == 0) len = strlen(buffer);
//  char *char852 = (char*)malloc(len+1);
  char char852[256];
  memcpy(char852,buffer,len+1);
  SlovakTo852(char852);
  if(!tft_is_text_mode)
	tft_textMode();
  tft_writeCommand(RA8875_MRWC);
  for (uint16_t i=0;i<len;i++)
  {
    tft_writeData(char852[i]);
    // This delay_ms is needed with textEnlarge(1) because
    // Teensy 3.X is much faster than Arduino Uno
//    if (tft_textScale > 0) delay_ms(1);
  }
//  free(char852);
}
void tft_textWrite(uint16_t left,uint16_t top,uint16_t right, uint16_t bottom, char* buffer, uint8_t align)
{
	#define charwidth 8
	#define charheight 16
	int len,text_width;
	char buff[105];
	len = strlen(buffer);
	memcpy(buff,buffer,len+1);
	if(right)
	{
		if(right < left)	//	Predpokladam ze je to sirka
			right+= left;
		if(len > (((right - left)* charwidth * (tft_textScale + 1)-3)))	//	Ak je text dlhsi ako sa zmesti do daneho priestoru
			buff[(right - left)* charwidth * (tft_textScale + 1)-3] = 0;
	}
	len = strlen(buff);
	if(right && ((align & 0x03) == ALINE_RIGHT))	//	Je zadany pravy okraj a chcem to zarovnat napravo
	{
		text_width = len * charwidth * (tft_textScale + 1);
		if(text_width < (right - left))
		{
			char chyba_medzier = ((right - left) - text_width) / (charwidth * (tft_textScale + 1)) + 3;	//	Zistim pocet medzier ktore doplnim na koniec
			sprintf(buff,"%s%*s",buffer,chyba_medzier,"");
		}
	}
	len = strlen(buff);
	text_width = len * charwidth * (tft_textScale + 1);	
	if(bottom != 0)
		top = top + ((bottom-top - (16 * (tft_textScale + 1))) / 2);
	switch (align & 0x03)
	{
		case ALINE_LEFT:	break;
		case ALINE_RIGHT:	if ((right - left) > text_width)	//	Je dost miesta, tak zarovnam vpravo
								left = right - text_width;
							break;
		case ALINE_CENTER:	if(text_width > (right - left))	//	Sirka textu je vacsia ako sirka vybraneho okna
							{	//	text rozdelim na dva riadky
								left = left + (right - left - text_width) / 2;
								unsigned char a;
								char buffer1[100];
								a = PosR(32,buff);
								if(a > 0)	//	Text obsahuje medzeru tak ho rozdelim na dva riadky
								{
									unsigned int left1,maxsize;
									memcpy(buffer1,buff,a);
									buffer1[a] = 0;
									maxsize =  strlen(buffer1) * charwidth * (tft_textScale + 1);
									left1 = left + (right - left - maxsize) / 2;
									tft_textSetCursor(left1 + ((tft_textScale + 1) * charwidth) / 2,top - charheight);
									tft_textWriteRaw(buffer1,0);
									memcpy(buffer1,buff + a + 1,strlen(buff)-a);
									left1 = left + (right - left - strlen(buffer1) * charwidth * (tft_textScale + 1)) / 2;
									tft_textSetCursor(left1 + ((tft_textScale + 1) * charwidth) / 2,top + charheight);
									tft_textWriteRaw(buffer1,0);
									return ;
								}
							}
							left = left + (right - left - text_width) / 2;
							break;

	}
	tft_textSetCursor(left,top);
	tft_textWriteRaw(buff,0);
}

/************************* Graphics ***********************************/

/**************************************************************************/
/*!
      Sets the display in graphics mode (as opposed to text mode)
*/
/**************************************************************************/
void tft_graphicsMode(void) {
  tft_writeCommand(RA8875_MWCR0);
  uint8_t temp = tft_readData();
  temp &= ~RA8875_MWCR0_TXTMODE; // bit #7
  tft_writeData(temp);
  tft_is_text_mode = false;
}

/**************************************************************************/
/*!
      Waits for screen to finish by polling the status!
*/
/**************************************************************************/
bool tft_waitPoll(uint8_t regname, uint8_t waitflag) {
  /* Wait for the command to finish */
  while (1)
  {
    uint8_t temp = tft_readReg(regname);
    if (!(temp & waitflag))
      return true;
  }  
  return false; // MEMEFIX: yeah i know, unreached! - add timeout?
}


/**************************************************************************/
/*!
      Sets the current X/Y position on the display before drawing
      
      @args x[in] The 0-based x location
      @args y[in] The 0-base y location
*/
/**************************************************************************/
void tft_setXY(uint16_t x, uint16_t y) {
  tft_writeReg(RA8875_CURH0, x);
  tft_writeReg(RA8875_CURH1, x >> 8);
  tft_writeReg(RA8875_CURV0, y);
  tft_writeReg(RA8875_CURV1, y >> 8);  
}

/**************************************************************************/
/*!
      HW accelerated function to push a chunk of raw pixel data
      
      @args num[in] The number of pixels to push
      @args p[in]   The pixel color to use
*/
/**************************************************************************/
void tft_pushPixels(uint32_t num, uint16_t p) {
 	while(!(PIOD->PIO_PDSR & (PIO_PD8)));	//	touch screen moze nacitavat
	lcd_spi_cs_low();
	LCD_SPIWrite(RA8875_DATAWRITE);
	while (num--) {
		LCD_SPIWrite(p >> 8);
		LCD_SPIWrite(p);
	}
	lcd_spi_cs_high();
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void tft_fillRect1(void) {
  tft_writeCommand(RA8875_DCR);
  tft_writeData(RA8875_DCR_LINESQUTRI_STOP | RA8875_DCR_DRAWSQUARE);
  tft_writeData(RA8875_DCR_LINESQUTRI_START | RA8875_DCR_FILL | RA8875_DCR_DRAWSQUARE);
}

/**************************************************************************/
/*!
      Draws a single pixel at the specified location

      @args x[in]     The 0-based x location
      @args y[in]     The 0-base y location
      @args color[in] The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void tft_drawPixel(int16_t x, int16_t y, uint16_t color)

{
	tft_writeReg(RA8875_CURH0, x & 0xFF);
	tft_writeReg(RA8875_CURH1, x >> 8);
	tft_writeReg(RA8875_CURV0, y & 0xFF);
	tft_writeReg(RA8875_CURV1, y >> 8);  
	tft_writeCommand(RA8875_MRWC);
	lcd_spi_cs_low();
	LCD_SPIWrite(RA8875_DATAWRITE);
	LCD_SPIWrite(color >> 8);
	LCD_SPIWrite(color);
	lcd_spi_cs_high();
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
void tft_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  /* Set X */
  tft_writeCommand(0x91);
  tft_writeData(x0);
  tft_writeCommand(0x92);
  tft_writeData(x0 >> 8);
  
  /* Set Y */
  tft_writeCommand(0x93);
  tft_writeData(y0); 
  tft_writeCommand(0x94);
  tft_writeData(y0 >> 8);
  
  /* Set X1 */
  tft_writeCommand(0x95);
  tft_writeData(x1);
  tft_writeCommand(0x96);
  tft_writeData((x1) >> 8);
  
  /* Set Y1 */
  tft_writeCommand(0x97);
  tft_writeData(y1); 
  tft_writeCommand(0x98);
  tft_writeData((y1) >> 8);
  
  /* Set Color */
  tft_writeCommand(0x63);
  tft_writeData((color & 0xf800) >> 11);
  tft_writeCommand(0x64);
  tft_writeData((color & 0x07e0) >> 5);
  tft_writeCommand(0x65);
  tft_writeData((color & 0x001f));

  /* Draw! */
  tft_writeCommand(RA8875_DCR);
  tft_writeData(0x80);
  
  /* Wait for the command to finish */
  tft_waitPoll(RA8875_DCR, RA8875_DCR_LINESQUTRI_STATUS);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void tft_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
  tft_drawLine(x, y, x, y+h, color);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void tft_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
  tft_drawLine(x, y, x+w, y, color);
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
void tft_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  tft_rectHelperRound(x, y, x+w, y+h, color, false,0);
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
void tft_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	tft_rectHelperRound(x, y, x+w, y+h, color, true,0);
}
void tft_fillRectRound(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color,uint8_t radius)
{
	tft_rectHelperRound(x, y, x+w, y+h, color, true,radius);
}

/**************************************************************************/
/*!
      Fills the screen with the spefied RGB565 color

      @args color[in] The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void tft_fillScreen(uint16_t color)
{  
  tft_rectHelperRound(0, 0, tft_width-1, tft_height-1, color, true,0);
  delay_ms(10);	//	Pockam kym prekresli celu obrazovku
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
void tft_drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
  tft_circleHelper(x0, y0, r, color, false);
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
void tft_fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
  tft_circleHelper(x0, y0, r, color, true);
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
void tft_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  tft_triangleHelper(x0, y0, x1, y1, x2, y2, color, false);
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
void tft_fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  tft_triangleHelper(x0, y0, x1, y1, x2, y2, color, true);
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
void tft_drawEllipse(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color)
{
  tft_ellipseHelper(xCenter, yCenter, longAxis, shortAxis, color, false);
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
void tft_fillEllipse(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color)
{
  tft_ellipseHelper(xCenter, yCenter, longAxis, shortAxis, color, true);
}

/**************************************************************************/
/*!
      Draws a HW accelerated curve on the display

      @args xCenter[in]   The 0-based x location of the ellipse's center
      @args yCenter[in]   The 0-based y location of the ellipse's center
      @args longAxis[in]  The size in pixels of the ellipse's long axis
      @args shortAxis[in] The size in pixels of the ellipse's short axis
      @args curvePart[in] The corner to draw, where in clock-wise motion:
                            0 = 180-270?                            1 = 270-0?                            2 = 0-90?                            3 = 90-180?      @args color[in]     The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void tft_drawCurve(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint8_t curvePart, uint16_t color)
{
  tft_curveHelper(xCenter, yCenter, longAxis, shortAxis, curvePart, color, false);
}

/**************************************************************************/
/*!
      Draws a HW accelerated filled curve on the display

      @args xCenter[in]   The 0-based x location of the ellipse's center
      @args yCenter[in]   The 0-based y location of the ellipse's center
      @args longAxis[in]  The size in pixels of the ellipse's long axis
      @args shortAxis[in] The size in pixels of the ellipse's short axis
      @args curvePart[in] The corner to draw, where in clock-wise motion:
                            0 = 180-270?                            1 = 270-0?                            2 = 0-90?                            3 = 90-180?      @args color[in]     The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void tft_fillCurve(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint8_t curvePart, uint16_t color)
{
  tft_curveHelper(xCenter, yCenter, longAxis, shortAxis, curvePart, color, true);
}

/**************************************************************************/
/*!
      Helper function for higher level circle drawing code
*/
/**************************************************************************/
void tft_circleHelper(int16_t x0, int16_t y0, int16_t r, uint16_t color, bool filled)
{
  /* Set X */
  tft_writeCommand(0x99);
  tft_writeData(x0);
  tft_writeCommand(0x9a);
  tft_writeData(x0 >> 8);
  
  /* Set Y */
  tft_writeCommand(0x9b);
  tft_writeData(y0); 
  tft_writeCommand(0x9c);	   
  tft_writeData(y0 >> 8);
  
  /* Set Radius */
  tft_writeCommand(0x9d);
  tft_writeData(r);  
  
  /* Set Color */
  tft_writeCommand(0x63);
  tft_writeData((color & 0xf800) >> 11);
  tft_writeCommand(0x64);
  tft_writeData((color & 0x07e0) >> 5);
  tft_writeCommand(0x65);
  tft_writeData((color & 0x001f));
  
  /* Draw! */
  tft_writeCommand(RA8875_DCR);
  if (filled)
  {
    tft_writeData(RA8875_DCR_CIRCLE_START | RA8875_DCR_FILL);
  }
  else
  {
    tft_writeData(RA8875_DCR_CIRCLE_START | RA8875_DCR_NOFILL);
  }
  
  /* Wait for the command to finish */
  tft_waitPoll(RA8875_DCR, RA8875_DCR_CIRCLE_STATUS);
}

/**************************************************************************/
/*!
      Helper function for higher level rectangle drawing code
*/
/**************************************************************************/
/*void tft_rectHelper(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, bool filled)
{
	tft_rectHelper(x,y,w,h,color,filled,false);
}*/

void tft_rectHelperRound(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, bool filled, uint8_t radius)
{
	tft_graphicsMode();
	/* Set X */
	tft_writeCommand(0x91);
	tft_writeData(x);
	tft_writeCommand(0x92);
	tft_writeData(x >> 8);
	
	/* Set Y */
	tft_writeCommand(0x93);
	tft_writeData(y);
	tft_writeCommand(0x94);
	tft_writeData(y >> 8);
	
	/* Set X1 */
	tft_writeCommand(0x95);
	tft_writeData(w);
	tft_writeCommand(0x96);
	tft_writeData((w) >> 8);
	
	/* Set Y1 */
	tft_writeCommand(0x97);
	tft_writeData(h);
	tft_writeCommand(0x98);
	tft_writeData((h) >> 8);
	/* Set Color */
	tft_writeCommand(0x63);
	tft_writeData((color & 0xf800) >> 11);
	tft_writeCommand(0x64);
	tft_writeData((color & 0x07e0) >> 5);
	tft_writeCommand(0x65);
	tft_writeData((color & 0x001f));

	/* Draw! */
	if(radius)
	{
		//	Set radius
		tft_writeCommand(0xA1);
		tft_writeData(radius);
		tft_writeCommand(0xA2);
		tft_writeData(radius >> 8);
		tft_writeCommand(0xA3);
		tft_writeData(radius);
		tft_writeCommand(0xA4);
		tft_writeData(radius >> 8);
		tft_writeCommand(RA8875_ELLIPSE);
		if (filled)
			tft_writeData(0xE0);
		else
			tft_writeData(0xA0);
		tft_waitPoll(RA8875_ELLIPSE, RA8875_ELLIPSE_STATUS);
	}else
	{
		tft_writeCommand(RA8875_DCR);
		if (filled)
			tft_writeData(0xB0);
		else
			tft_writeData(0x90);
		tft_waitPoll(RA8875_DCR, RA8875_DCR_LINESQUTRI_STATUS);
	}
	
	/* Wait for the command to finish */
	  /* Wait for the command to finish */
}

void tft_graphicCursorType(uint8_t typ)
{
	int i;	
	if(tft_cursor_type == typ) return;
	uint8_t const target[256]  ={0xAA,0xAA,0xAA,0xAB,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAB,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAB,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAB,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xBF,0xFA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAB,0xEB,0xAF,0xAA,0xAA,0xAA,0xAA,0xAA,0xBE,0xAB,0xAA,0xFA,0xAA,0xAA,0xAA,0xAA,0xEA,0xAB,0xAA,0xAE,0xAA,0xAA,0xAA,0xAB,0xAA,0xAB,0xAA,0xAB,0xAA,0xAA,0xAA,0xAB,0xAA,0xAB,0xAA,0xAB,0xAA,0xAA,0xAA,0xAE,0xAA,0xAB,0xAA,0xAA,0xEA,0xAA,0xAA,0xAE,0xAA,0xAB,0xAA,0xAA,0xEA,0xAA,0xAA,0xBA,0xAA,0xAB,0xAA,0xAA,0xBA,0xAA,0xAA,0xBA,0xAA,0xAB,0xAA,0xAA,0xBA,0xAA,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xAA,0xBA,0xAA,0xAB,0xAA,0xAA,0xBA,0xAA,0xAA,0xBA,0xAA,0xAB,0xAA,0xAA,0xBA,0xAA,0xAA,0xBA,0xAA,0xAB,0xAA,0xAA,0xBA,0xAA,0xAA,0xAE,0xAA,0xAB,0xAA,0xAA,0xEA,0xAA,0xAA,0xAE,0xAA,0xAB,0xAA,0xAA,0xEA,0xAA,0xAA,0xAB,0xAA,0xAB,0xAA,0xAB,0xAA,0xAA,0xAA,0xAB,0xAA,0xAB,0xAA,0xAB,0xAA,0xAA,0xAA,0xAA,0xEA,0xAB,0xAA,0xAE,0xAA,0xAA,0xAA,0xAA,0xBE,0xAB,0xAA,0xFA,0xAA,0xAA,0xAA,0xAA,0xAB,0xEB,0xAF,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xBF,0xFA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAB,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAB,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAB,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAB,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAB,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};
	uint8_t const arrow[256]   ={0xEA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xFA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xCE,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xC3,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xC0,0xEA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xC0,0x3A,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xC0,0x0E,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xC0,0x03,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xC0,0x00,0xEA,0xAA,0xAA,0xAA,0xAA,0xAA,0xC0,0x00,0x3A,0xAA,0xAA,0xAA,0xAA,0xAA,0xC0,0x00,0x0E,0xAA,0xAA,0xAA,0xAA,0xAA,0xC0,0x00,0x03,0xAA,0xAA,0xAA,0xAA,0xAA,0xC0,0x00,0x00,0xEA,0xAA,0xAA,0xAA,0xAA,0xC0,0x00,0x00,0x3A,0xAA,0xAA,0xAA,0xAA,0xC0,0x00,0xFF,0xFE,0xAA,0xAA,0xAA,0xAA,0xC0,0x30,0xEA,0xAA,0xAA,0xAA,0xAA,0xAA,0xC0,0xF0,0x3A,0xAA,0xAA,0xAA,0xAA,0xAA,0xC3,0xAC,0x3A,0xAA,0xAA,0xAA,0xAA,0xAA,0xCE,0xAC,0x0E,0xAA,0xAA,0xAA,0xAA,0xAA,0xFA,0xAB,0x0E,0xAA,0xAA,0xAA,0xAA,0xAA,0xEA,0xAB,0x03,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xC3,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xC3,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xBF,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};
//	graphicCursorDisable();
	tft_writeCommand(0x41);
	tft_writeData(0x08);	//	Adresa je kurzor
	tft_writeCommand(0x02);	//	Memory write
	if(typ == 1)
		for(i = 0; i < 256; i++)
		tft_writeData(arrow[i]);
	if(typ == 2)
		for(i = 0; i < 256; i++)
			tft_writeData(target[i]);
	tft_cursor_type = typ;
	tft_writeCommand(0x41);
	tft_writeData(0x00);	//	Adresa je LAYER
	tft_graphicCursorEnable();
}
void tft_graphicCursorEnable(void)
{
	tft_writeCommand(0x41);
	tft_writeData(1 << 7);	//	Povolim kurzor
}
void tft_graphicCursorDisable(void)
{
	tft_writeCommand(0x41);
	tft_writeData(0x00);	//	Zakazem kurzor
}

void tft_graphicCursor(uint16_t x, uint16_t y)
{
	if(tft_cursor_type == 2)
	{
		x -= 15;
		if(x > 20000)	//	overflow
			x = 0;
		y -= 15;
		if(y > 20000)	//	overflow
			y = 0;
	}
	/* Set cursor location */
	tft_writeCommand(0x80);	//	X low
	tft_writeData(x & 0xFF);
	tft_writeCommand(0x81);	//	X high
	tft_writeData(x >> 8);
	tft_writeCommand(0x82);	//	Y low
	tft_writeData(y & 0xFF);
	tft_writeCommand(0x83);	//	Y high
	tft_writeData(y >> 8);
	tft_writeCommand(0x84);	//	farba  0 kurzoru RRRGGGBB
	tft_writeData(0xE0);	//	RED
	tft_writeCommand(0x85);	//	farba  1 kurzoru RRRGGGBB
	tft_writeData(0x07 << 2); // GREEN
}


/**************************************************************************/
/*!
      Helper function for higher level triangle drawing code
*/
/**************************************************************************/
void tft_triangleHelper(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color, bool filled)
{
  /* Set Point 0 */
  tft_writeCommand(0x91);
  tft_writeData(x0);
  tft_writeCommand(0x92);
  tft_writeData(x0 >> 8);
  tft_writeCommand(0x93);
  tft_writeData(y0); 
  tft_writeCommand(0x94);
  tft_writeData(y0 >> 8);

  /* Set Point 1 */
  tft_writeCommand(0x95);
  tft_writeData(x1);
  tft_writeCommand(0x96);
  tft_writeData(x1 >> 8);
  tft_writeCommand(0x97);
  tft_writeData(y1); 
  tft_writeCommand(0x98);
  tft_writeData(y1 >> 8);

  /* Set Point 2 */
  tft_writeCommand(0xA9);
  tft_writeData(x2);
  tft_writeCommand(0xAA);
  tft_writeData(x2 >> 8);
  tft_writeCommand(0xAB);
  tft_writeData(y2); 
  tft_writeCommand(0xAC);
  tft_writeData(y2 >> 8);
  
  /* Set Color */
  tft_writeCommand(0x63);
  tft_writeData((color & 0xf800) >> 11);
  tft_writeCommand(0x64);
  tft_writeData((color & 0x07e0) >> 5);
  tft_writeCommand(0x65);
  tft_writeData((color & 0x001f));
  
  /* Draw! */
  tft_writeCommand(RA8875_DCR);
  if (filled)
  {
    tft_writeData(0xA1);
  }
  else
  {
    tft_writeData(0x81);
  }
  
  /* Wait for the command to finish */
  tft_waitPoll(RA8875_DCR, RA8875_DCR_LINESQUTRI_STATUS);
}

/**************************************************************************/
/*!
      Helper function for higher level ellipse drawing code
*/
/**************************************************************************/
void tft_ellipseHelper(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color, bool filled)
{
  /* Set Center Point */
  tft_writeCommand(0xA5);
  tft_writeData(xCenter);
  tft_writeCommand(0xA6);
  tft_writeData(xCenter >> 8);
  tft_writeCommand(0xA7);
  tft_writeData(yCenter); 
  tft_writeCommand(0xA8);
  tft_writeData(yCenter >> 8);

  /* Set Long and Short Axis */
  tft_writeCommand(0xA1);
  tft_writeData(longAxis);
  tft_writeCommand(0xA2);
  tft_writeData(longAxis >> 8);
  tft_writeCommand(0xA3);
  tft_writeData(shortAxis); 
  tft_writeCommand(0xA4);
  tft_writeData(shortAxis >> 8);
  
  /* Set Color */
  tft_writeCommand(0x63);
  tft_writeData((color & 0xf800) >> 11);
  tft_writeCommand(0x64);
  tft_writeData((color & 0x07e0) >> 5);
  tft_writeCommand(0x65);
  tft_writeData((color & 0x001f));
  
  /* Draw! */
  tft_writeCommand(0xA0);
  if (filled)
  {
    tft_writeData(0xC0);
  }
  else
  {
    tft_writeData(0x80);
  }
  
  /* Wait for the command to finish */
  tft_waitPoll(RA8875_ELLIPSE, RA8875_ELLIPSE_STATUS);
}

/**************************************************************************/
/*!
      Helper function for higher level curve drawing code
*/
/**************************************************************************/
void tft_curveHelper(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint8_t curvePart, uint16_t color, bool filled)
{
  /* Set Center Point */
  tft_writeCommand(0xA5);
  tft_writeData(xCenter);
  tft_writeCommand(0xA6);
  tft_writeData(xCenter >> 8);
  tft_writeCommand(0xA7);
  tft_writeData(yCenter); 
  tft_writeCommand(0xA8);
  tft_writeData(yCenter >> 8);

  /* Set Long and Short Axis */
  tft_writeCommand(0xA1);
  tft_writeData(longAxis);
  tft_writeCommand(0xA2);
  tft_writeData(longAxis >> 8);
  tft_writeCommand(0xA3);
  tft_writeData(shortAxis); 
  tft_writeCommand(0xA4);
  tft_writeData(shortAxis >> 8);
  
  /* Set Color */
  tft_writeCommand(0x63);
  tft_writeData((color & 0xf800) >> 11);
  tft_writeCommand(0x64);
  tft_writeData((color & 0x07e0) >> 5);
  tft_writeCommand(0x65);
  tft_writeData((color & 0x001f));

  /* Draw! */
  tft_writeCommand(0xA0);
  if (filled)
  {
    tft_writeData(0xD0 | (curvePart & 0x03));
  }
  else
  {
    tft_writeData(0x90 | (curvePart & 0x03));
  }
  
  /* Wait for the command to finish */
  tft_waitPoll(RA8875_ELLIPSE, RA8875_ELLIPSE_STATUS);
}

/************************* Mid Level ***********************************/

/**************************************************************************/
/*!

*/
/**************************************************************************/
void tft_GPIOX(bool on) {
  if (on)
    tft_writeReg(RA8875_GPIOX, 1);
  else 
    tft_writeReg(RA8875_GPIOX, 0);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void tft_PWM1out(uint8_t p) {
  tft_writeReg(RA8875_P1DCR, p);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void tft_PWM2out(uint8_t p) {
  tft_writeReg(RA8875_P2DCR, p);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void tft_PWM1config(bool on, uint8_t clock) {
  if (on) {
    tft_writeReg(RA8875_P1CR, RA8875_P1CR_ENABLE | (clock & 0xF));
  } else {
    tft_writeReg(RA8875_P1CR, RA8875_P1CR_DISABLE | (clock & 0xF));
  }
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void tft_PWM2config(bool on, uint8_t clock) {
  if (on) {
    tft_writeReg(RA8875_P2CR, RA8875_P2CR_ENABLE | (clock & 0xF));
  } else {
    tft_writeReg(RA8875_P2CR, RA8875_P2CR_DISABLE | (clock & 0xF));
  }
}

/**************************************************************************/
/*!
      Enables or disables the on-chip touch screen controller
*/
/**************************************************************************/
void tft_touchEnable(bool on) 
{
  if (on) 
  {
    /* Enable Touch Panel (Reg 0x70) */
    tft_writeReg(RA8875_TPCR0, RA8875_TPCR0_ENABLE        | 
                           RA8875_TPCR0_WAIT_4096CLK  |
                           RA8875_TPCR0_WAKEDISABLE   | 
                           RA8875_TPCR0_ADCCLK_DIV4); // 10mhz max!
    /* Set Auto Mode      (Reg 0x71) */
    tft_writeReg(RA8875_TPCR1, RA8875_TPCR1_AUTO    | 
                           // RA8875_TPCR1_VREFEXT | 
                           RA8875_TPCR1_DEBOUNCE);
    /* Enable TP INT */
    tft_writeReg(RA8875_INTC1, tft_readReg(RA8875_INTC1) | RA8875_INTC1_TP);
  } 
  else
  {
    /* Disable TP INT */
    tft_writeReg(RA8875_INTC1, tft_readReg(RA8875_INTC1) & ~RA8875_INTC1_TP);
    /* Disable Touch Panel (Reg 0x70) */
    tft_writeReg(RA8875_TPCR0, RA8875_TPCR0_DISABLE);
  }
}

/**************************************************************************/
/*!
      Checks if a touch event has occured
      
      @returns  True is a touch event has occured (reading it via
                touchRead() will clear the interrupt in memory)
*/
/**************************************************************************/
bool tft_istouched(void) 
{
	volatile int touch_status;
	touch_status = tft_readReg(RA8875_INTC2) & RA8875_INTC2_TP;
	if (touch_status == RA8875_INTC2_TP) return true;
	return false;
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
bool tft_touchRead(uint16_t *x, uint16_t *y) 
{
  uint16_t tx, ty;
  uint8_t temp;
  
  tx = tft_readReg(RA8875_TPXH);
  ty = tft_readReg(RA8875_TPYH);
  temp = tft_readReg(RA8875_TPXYL);
  tx <<= 2;
  ty <<= 2;
  tx |= temp & 0x03;        // get the bottom x bits
  ty |= (temp >> 2) & 0x03; // get the bottom y bits

  *x = tx;
  *y = ty;

  /* Clear TP INT Status */
  tft_writeReg(RA8875_INTC2, RA8875_INTC2_TP);

  return true;
}

/**************************************************************************/
/*!
      Turns the display on or off
*/
/**************************************************************************/
void tft_displayOn(bool on) 
{
 if (on) 
   tft_writeReg(RA8875_PWRR, RA8875_PWRR_NORMAL | RA8875_PWRR_DISPON);
 else
   tft_writeReg(RA8875_PWRR, RA8875_PWRR_NORMAL | RA8875_PWRR_DISPOFF);
}

/**************************************************************************/
/*!
    Puts the display in sleep mode, or disables sleep mode if enabled
*/
/**************************************************************************/
void tft_sleep(bool sleep_lcd) 
{
 if (sleep_lcd) 
   tft_writeReg(RA8875_PWRR, RA8875_PWRR_DISPOFF | RA8875_PWRR_SLEEP);
 else
   tft_writeReg(RA8875_PWRR, RA8875_PWRR_DISPOFF);
}

/************************* Low Level ***********************************/

/**************************************************************************/
/*!

*/
/**************************************************************************/
void  tft_writeReg(uint8_t reg, uint8_t val) 
{
  tft_writeCommand(reg);
  tft_writeData(val);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
uint8_t  tft_readReg(uint8_t reg) 
{
  tft_writeCommand(reg);
  return tft_readData();
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void  tft_writeData(uint8_t d) 
{
 	while(!(PIOD->PIO_PDSR & (PIO_PD8)));	//	touch screen moze nacitavat
	lcd_spi_cs_low();
	LCD_SPIWrite(RA8875_DATAWRITE);
	LCD_SPIWrite(d);
	lcd_spi_cs_high();
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
uint8_t  tft_readData(void) 
{
 	while(!(PIOD->PIO_PDSR & (PIO_PD8)));	//	touch screen moze nacitavat
	lcd_spi_cs_low();
	LCD_SPIWrite(RA8875_DATAREAD);
	uint8_t x = LCD_SPIWrite(0x0);
	lcd_spi_cs_high();
	return x;
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void  tft_writeCommand(uint8_t d) 
{
 	while(!(PIOD->PIO_PDSR & (PIO_PD8)));	//	touch screen moze nacitavat
	lcd_spi_cs_low();
	LCD_SPIWrite(RA8875_CMDWRITE);
	LCD_SPIWrite(d);
	lcd_spi_cs_high();
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
uint8_t  tft_readStatus(void) 
{
 	while(!(PIOD->PIO_PDSR & (PIO_PD8)));	//	touch screen moze nacitavat
	lcd_spi_cs_low();
	LCD_SPIWrite(RA8875_CMDREAD);
	uint8_t x = LCD_SPIWrite(0x0);
	lcd_spi_cs_high();
	return x;
}
void tft_caption_draw(unsigned char handle)
{
#define TRIANGLE_SIZE 15	
#define CIRCLE_SIZE 30
	TGraphics *p;
	p = Gall;
	int center_x,center_y;
	do
	{
		if(p->handle == handle)
		{
//			SetColor(p->BackColor);
//			fillRect(p->Rect.x,p->Rect.y,p->Rect.width,p->Rect.height,p->BackColor);
			tft_textMode();
			tft_textEnlarge(1);
//			textTransparent(p->BackColor);
			tft_textColor(p->Color,p->BackColor);
			if(p->type == 1)
			{
				center_x = p->Rect.x + p->Rect.width / 2;
				center_y = p->Rect.y + p->Rect.height / 2;
				if(!strncmp(p->text,"~DOWN",5))
					tft_fillTriangle(center_x-TRIANGLE_SIZE,center_y-TRIANGLE_SIZE,center_x+TRIANGLE_SIZE,center_y-TRIANGLE_SIZE,center_x,center_y+TRIANGLE_SIZE,RA8875_BLACK);
				else if(!strncmp(p->text,"~UP",3))
					tft_fillTriangle(center_x-TRIANGLE_SIZE,center_y+TRIANGLE_SIZE,center_x+TRIANGLE_SIZE,center_y+TRIANGLE_SIZE,center_x,center_y-TRIANGLE_SIZE,RA8875_BLACK);
				else if(!strncmp(p->text,"~RIGHT",6))
					tft_fillTriangle(center_x-TRIANGLE_SIZE,center_y-TRIANGLE_SIZE,center_x-TRIANGLE_SIZE,center_y+TRIANGLE_SIZE,center_x + TRIANGLE_SIZE ,center_y,RA8875_BLACK);
				else if(!strncmp(p->text,"~LEFT",6))
					tft_fillTriangle(center_x+TRIANGLE_SIZE,center_y-TRIANGLE_SIZE,center_x+TRIANGLE_SIZE,center_y+TRIANGLE_SIZE,center_x - TRIANGLE_SIZE ,center_y,RA8875_BLACK);
				else if(!strncmp(p->text,"~CW",3))
				{
					tft_drawCircle(center_x,center_y,CIRCLE_SIZE,RA8875_BLACK);
					tft_drawCircle(center_x,center_y,CIRCLE_SIZE + 1,RA8875_BLACK);
					center_x += CIRCLE_SIZE;
					tft_fillTriangle(center_x-TRIANGLE_SIZE / 3,center_y-TRIANGLE_SIZE / 3,center_x+TRIANGLE_SIZE / 3,center_y-TRIANGLE_SIZE / 3,center_x,center_y+TRIANGLE_SIZE / 3,RA8875_BLACK);
				}else if(!strncmp(p->text,"~CCW",4))
				{
					tft_drawCircle(center_x,center_y,CIRCLE_SIZE,RA8875_BLACK);
					tft_drawCircle(center_x,center_y,CIRCLE_SIZE + 1,RA8875_BLACK);
					center_x -= CIRCLE_SIZE;
					tft_fillTriangle(center_x-TRIANGLE_SIZE / 3,center_y-TRIANGLE_SIZE / 3,center_x+TRIANGLE_SIZE / 3,center_y-TRIANGLE_SIZE / 3,center_x,center_y+TRIANGLE_SIZE / 3,RA8875_BLACK);
				}

				else tft_textWrite(p->Rect.x+3,p->Rect.y,p->Rect.x + p->Rect.width-3,p->Rect.y + p->Rect.height,p->text,ALINE_CENTER);	// button
			}
			if(p->type == 4)
				tft_textWrite(p->Rect.x+3,p->Rect.y,p->Rect.x + p->Rect.width-3,p->Rect.y + p->Rect.height,p->text,ALINE_CENTER);	// button 1
			else if(p->type == 2)
				tft_textWrite(p->Rect.x,p->Rect.y,p->Rect.x + p->Rect.width,p->Rect.y + p->Rect.height,p->text,ALINE_RIGHT);	// check box
			break;
		}
		p = p->next;
	}while(p);
}

void tft_button(unsigned int x,unsigned int y,unsigned int width1,unsigned int height1,const char *text, unsigned char handle, uint16_t color,uint8_t radius)
{
	//	save_colors();
	tft_add_object(x,y,width1,height1,handle,1,0,color,RA8875_BLACK,text,radius);
	tft_frame_draw(handle);
	tft_caption_draw(handle);
	//	restore_colors();
}

void tft_redraw_all(void)
{
	TGraphics *p;
	p = Gall;
	do
	{
		tft_frame_draw(p->handle);
		tft_caption_draw(p->handle);
		p = p->next;
	}while(p);

}
#define ROUNDED_BUTTONS
void tft_frame_draw(unsigned char handle)
{
	TGraphics *p;
	unsigned int x,width1;
	unsigned int y,height1;
	
	p = Gall;
	do
	{
		if(p->handle == handle)
		{
			x = p->Rect.x;
			y = p->Rect.y;
			width1 = p->Rect.width;
			height1 = p->Rect.height;
			if(p->type == 2)	//	Checkbox
			{
				width1 = 15;
				height1 = 15;
			}
			if(p->value != 0)
			{
				if(p->radius)
				{
					tft_fillRectRound(x+1,y+1,width1+1,height1+1,RA885_LIGHTGRAY,p->radius);
					tft_fillRectRound(x,y,width1,height1,p->BackColor,p->radius);
				} else
				{
					tft_fillRect(x,y + height1,width1,1,RA885_LIGHTGRAY);
					tft_fillRect(x,y,1,height1,RA885_LIGHTGRAY);
					tft_fillRect(x,y,width1,1,p->BackColor);
					tft_fillRect(x+width1,y,1,height1,p->BackColor);
				}
			}else
			{
				if(p->radius)
				{
					tft_fillRectRound(x,y,width1,height1,RA885_LIGHTGRAY,p->radius);
					tft_fillRectRound(x+1,y+1,width1+1,height1+1,p->BackColor,p->radius);
				} else
				{
					tft_fillRect(x,y,width1,height1,p->BackColor);
					tft_fillRect(x,y,1,height1,p->BackColor);
					tft_fillRect(x+1,y,width1-2,1,RA885_LIGHTGRAY);
					tft_fillRect(x+width1,y+1,1,height1-2,RA885_LIGHTGRAY);
				}
			};
			break;
		}
		p = p->next;
	}while(p);
}

uint8_t tft_get_object(unsigned char handle,TGraphics *g)	//	Nefunguje spravne !!!!!!!!!
{
	TGraphics *p;
	p = Gall;
	do
	{
		if(p->handle == handle)
		{
			g = p;
			return 1;
		}
		p = p->next;
	}while(p);
	return 0;
}

uint8_t tft_change_text(unsigned char handle,const char * text)
{
	TGraphics *p;
	p = Gall;
	do
	{
		if(p->handle == handle)
		{
			free(p->text);
			p->text = (char*)malloc(strlen(text) + 1);
			memcpy(p->text,text,strlen(text) + 1);
			return 1;
		}
		p = p->next;
	}while(p);
	return 0;
}

uint8_t tft_touched(unsigned int x, unsigned int y)
{
	uint8_t result = 0;
	TGraphics *p;
	p = Gall;
	do
	{
		if( (x >= p->Rect.x) && (y >= p->Rect.y) && (x <= (p->Rect.x + p->Rect.width)) && (y <= (p->Rect.y + p->Rect.height)))
		{
			IsTouched = 1;
			result = 1;
			if(!realtime)
			{
				if(p->type == 1)	//	tlacitko
				{
					p->value = 1;
//					save_colors();
					tft_frame_draw(p->handle); 
					tft_textMode();
					tft_textColor(p->Color,p->BackColor);
					tft_textEnlarge(1);
					int center_x = p->Rect.x + p->Rect.width / 2;
					int center_y = p->Rect.y + p->Rect.height / 2 + 2;
					if(!strncmp(p->text,"~DOWN",5))
						tft_fillTriangle(center_x-TRIANGLE_SIZE,center_y-TRIANGLE_SIZE,center_x+TRIANGLE_SIZE,center_y-TRIANGLE_SIZE,center_x,center_y+TRIANGLE_SIZE,RA8875_BLACK);
					else if(!strncmp(p->text,"~UP",3))
						tft_fillTriangle(center_x-TRIANGLE_SIZE,center_y+TRIANGLE_SIZE,center_x+TRIANGLE_SIZE,center_y+TRIANGLE_SIZE,center_x,center_y-TRIANGLE_SIZE,RA8875_BLACK);
					else if(!strncmp(p->text,"~RIGHT",6))
						tft_fillTriangle(center_x-TRIANGLE_SIZE,center_y-TRIANGLE_SIZE,center_x-TRIANGLE_SIZE,center_y+TRIANGLE_SIZE,center_x + TRIANGLE_SIZE ,center_y,RA8875_BLACK);
					else if(!strncmp(p->text,"~LEFT",6))
						tft_fillTriangle(center_x+TRIANGLE_SIZE,center_y-TRIANGLE_SIZE,center_x+TRIANGLE_SIZE,center_y+TRIANGLE_SIZE,center_x - TRIANGLE_SIZE ,center_y,RA8875_BLACK);
					else if(!strncmp(p->text,"~CW",3))
					{
						tft_drawCircle(center_x,center_y,CIRCLE_SIZE,RA8875_BLACK);
						tft_drawCircle(center_x,center_y,CIRCLE_SIZE + 1,RA8875_BLACK);
						center_x += CIRCLE_SIZE;
						tft_fillTriangle(center_x-TRIANGLE_SIZE / 3,center_y-TRIANGLE_SIZE / 3,center_x+TRIANGLE_SIZE / 3,center_y-TRIANGLE_SIZE / 3,center_x,center_y+TRIANGLE_SIZE / 3,RA8875_BLACK);
					}else if(!strncmp(p->text,"~CCW",4))
					{
						tft_drawCircle(center_x,center_y,CIRCLE_SIZE,RA8875_BLACK);
						tft_drawCircle(center_x,center_y,CIRCLE_SIZE + 1,RA8875_BLACK);
						center_x += CIRCLE_SIZE;
						tft_fillTriangle(center_x-TRIANGLE_SIZE / 3,center_y-TRIANGLE_SIZE / 3,center_x+TRIANGLE_SIZE / 3,center_y-TRIANGLE_SIZE / 3,center_x,center_y+TRIANGLE_SIZE / 3,RA8875_BLACK);
					}
					else
						tft_textWrite(p->Rect.x+3,p->Rect.y+2,p->Rect.x + p->Rect.width-3,p->Rect.y + p->Rect.height+2,p->text,ALINE_CENTER);	// button
//				restore_colors();
				}
				if(p->type == 4)	//	tlacitko1
				{
					if(p->value)
						p->value = 0;
					else
						p->value = 1;
//					save_colors();
					tft_frame_draw(p->handle);
					tft_textMode();
					tft_textColor(p->Color,p->BackColor);
					tft_textEnlarge(1);
					tft_textWrite(p->Rect.x+3,p->Rect.y+2,p->Rect.x + p->Rect.width-3,p->Rect.y + p->Rect.height+2,p->text,ALINE_CENTER);	// button
//					restore_colors();
				}
				if(p->type == 2)	//	check box
				{
					if(p->value)
					p->value = 0;
					else
					p->value = 1;
//					save_colors();
//					chbvalue_draw(p->Rect.x,p->Rect.y,p->value);
//					restore_colors();
				}
				if(p->type == 3)	//	Trackbar
				{
					p->value = (x - p->Rect.x) * 100 / p->Rect.width;
//					save_colors();
//					trackbardraw(p->Rect.x,p->Rect.y,p->Rect.width,p->Rect.height,p->value,p->BackColor,p->Color);
//					restore_colors();
				}
			}
			dotyk = 1;
			LastTouchXPos = x;
			LastTouchYPos = y;
			Action = p->handle;
//			beep(10);
		}
		p = p->next;
	}while(p);
	return result;
}
void tft_untouched(unsigned int x, unsigned int y)
{
	TGraphics *p;
	p = Gall;
	do
	{
		if( (x >= p->Rect.x) && (y >= p->Rect.y) && (x <= (p->Rect.x + p->Rect.width)) && (y <= (p->Rect.y + p->Rect.height)))
		{
			IsTouched = 0;
			if(!realtime)
			{
				if(p->type == 1)	//	tlacitko
				{
					p->value = 0;
//					save_colors();
					tft_frame_draw(p->handle);
					tft_caption_draw(p->handle);
//					restore_colors();
				}
			}
			dotyk = 0;
			Action1 = p->handle;
		}
		p = p->next;
	}while(p);
}

void tft_draw_press_unpress(void)
{
	tft_GetTouch();
	if((TouchXPos != 0) && (TouchYPos != 0))
	{
		if(!IsTouched)
		{
			if(tft_touched(TouchXPos,TouchYPos))	// ak som klikol niekde kde to zije
			{
				if(!realtime)
					beep(10);
				tft_graphicCursor(TouchXPos,TouchYPos);
				tft_graphicCursorType(1);
				tft_graphicCursorEnable();
			}
		}
		TouchXPos = 0;
		TouchYPos = 0;
	}
	if(unpressed)
	{
		unpressed=false;
		tft_untouched(LastTouchXPos,LastTouchYPos);
	}
/*	if(need_joystick)
	{
		int x,y;
		need_joystick = false;
		joystick_read(&x,&y);
	}*/
}

void tft_delete_all_objects(void)
{
	TGraphics *temp;
	temp = Gall;	//	temp ukazuje na prvyv zozname=>posledne pridany objekt
	Action1 = 0;
	Action = 0;
	while(temp)
	{
		Gall=temp->next;
		free(temp->text);
		free(temp);
		temp = Gall;
	}
	tft_graphicCursorDisable();
}
void tft_add_object(unsigned int x, unsigned int y, unsigned int width1, unsigned int height1, unsigned char handle, unsigned char type, unsigned char value,unsigned int BackColor,unsigned int Color, const char *text, char radius)
{
	TGraphics *p;
//	p = new(TGraphics);

	p = (TGraphics*)malloc(sizeof(TGraphics));
	if((p ==NULL) || (((p->text =(char*)malloc(strlen(text)+1))==NULL)))
	{
		tft_fillScreen(RA8875_RED);
		tft_textColor(RA8875_BLACK,RA8875_RED);
		tft_textEnlarge(2);
		tft_textWrite(0,30, 800, 80, (char*)"malloc() error. No Space", ALINE_CENTER);
		while(1);
	}
	//	mem_use1 += sizeof(struct TGraphics) + strlen(text)+1;
	p->Rect.x = x;
	p->Rect.y = y;
	p->Rect.width = width1;
	p->Rect.height = height1;
	p->next = NULL;
	p->handle = handle;
	p->type = type;
	p->value = value;
	p->BackColor = BackColor;
	p->Color = Color;
	p->radius = radius;
	memcpy(p->text,text,strlen(text)+1);
	//	p->text = text;
	if(Gall == NULL)
	{
		Gall = Gcurrent = p;
	}else
	{
		p->next = Gall;
		Gall = p;
	}
}
void tft_drawcalcbuttons(void)
{	//	1 
	tft_button(CalcX(10,4,1),CalcY(10,4,4),CalcHeight(10,4),CalcHeight(10,4),"0",10+0,RA8875_LIME,10);
	tft_button(CalcX(10,4,3),CalcY(10,4,4),CalcHeight(10,4),CalcHeight(10,4),"<-",3  ,RA8875_LIME,10);
	tft_button(CalcX(10,4,1),CalcY(10,4,3),CalcHeight(10,4),CalcHeight(10,4),"1",10+1,RA8875_LIME,10);
	tft_button(CalcX(10,4,2),CalcY(10,4,3),CalcHeight(10,4),CalcHeight(10,4),"2",10+2,RA8875_LIME,10);
	tft_button(CalcX(10,4,3),CalcY(10,4,3),CalcHeight(10,4),CalcHeight(10,4),"3",10+3,RA8875_LIME,10);
	tft_button(CalcX(10,4,1),CalcY(10,4,2),CalcHeight(10,4),CalcHeight(10,4),"4",10+4,RA8875_LIME,10);
	tft_button(CalcX(10,4,2),CalcY(10,4,2),CalcHeight(10,4),CalcHeight(10,4),"5",10+5,RA8875_LIME,10);
	tft_button(CalcX(10,4,3),CalcY(10,4,2),CalcHeight(10,4),CalcHeight(10,4),"6",10+6,RA8875_LIME,10);
	tft_button(CalcX(10,4,1),CalcY(10,4,1),CalcHeight(10,4),CalcHeight(10,4),"7",10+7,RA8875_LIME,10);
	tft_button(CalcX(10,4,2),CalcY(10,4,1),CalcHeight(10,4),CalcHeight(10,4),"8",10+8,RA8875_LIME,10);
	tft_button(CalcX(10,4,3),CalcY(10,4,1),CalcHeight(10,4),CalcHeight(10,4),"9",10+9,RA8875_LIME,10);
	tft_button(400,CalcY(10,4,4),300,CalcHeight(10,4),"OK",1,RA8875_GREEN,10);
	tft_button(400,CalcY(10,4,3),300,CalcHeight(10,4),"Zruöiù",2,RA8875_RED,10);
	tft_button(CalcX(10,4,2),CalcY(10,4,4),CalcHeight(10,4),CalcHeight(10,4),",",4   ,RA8875_LIME,10);
	
}
/*float tft_GetFloat(const char *Text,float defval, float min, float max)
{
	char tmp;
	return tft_GetFloat(Text,defval, min, max,&tmp);
}*/

float tft_GetFloat(const char *Text,float defval, float min, float max, char *result)
{
	char buffer[100],first=1;
	int bufferpos;
	tft_delete_all_objects();
	tft_fillScreen(RA8875_BLACK);
	tft_drawcalcbuttons();	//	Vykreslim klavesnicu 0-9,....
	if(min < 0)	//	Pouzitelne su aj zaporne cisla
		tft_button(CalcX(10,4,4),CalcY(10,4,1),CalcHeight(10,4),CalcHeight(10,4),"-",5,RA8875_LIME,10);
	tft_textMode();
	tft_textEnlarge(1);
	tft_textColor(RA8875_YELLOW,RA8875_BLACK);
	tft_textWrite(500,10,0,0, (char*)Text, ALINE_LEFT);
	tft_graphicsMode();
	tft_fillRect(500,100,150,50,RA8875_YELLOW);
	bufferpos = sprintf(buffer,"%6.3f",defval);
	tft_textColor(RA8875_BLACK,RA8875_YELLOW);
	tft_textMode();
	tft_textWrite(500,100,140,0, buffer, ALINE_LEFT);
	Action1 = 0;
	while(1)
	{
		tft_draw_press_unpress();
		//		FontNo=FONT_DIGI;
		if(Action1 == 3)	//	Pressed backspace
		{
			if(bufferpos > 0)
			{
				bufferpos--;
				buffer[bufferpos] = 0;
			}
			tft_fillRect(500,100,150,50,RA8875_YELLOW);
			tft_textMode();
			tft_textColor(RA8875_BLACK,RA8875_YELLOW);
			tft_textWrite(500,100,140,0, buffer, ALINE_LEFT);
			Action1 = 0;
		}
		if(Action1 >= 10)	//	Pressed number 0-9
		{
			if(first)
			{
				first = 0;
				bufferpos = 0;
				memset(buffer,0,sizeof(buffer));
			}
			if(bufferpos < 9)
			{
				buffer[bufferpos] = 48 + Action - 10;
				bufferpos++;
			}
			tft_fillRect(500,100,150,50,RA8875_YELLOW);
			tft_textMode();
			tft_textColor(RA8875_BLACK,RA8875_YELLOW);
			tft_textWrite(500,100,140,0, buffer, ALINE_LEFT);
			Action1 = 0;
		}
		if(Action1 == 4)	//	  ciarka
		{
			if(first)
			{
				first = 0;
				bufferpos = 0;
				memset(buffer,0,sizeof(buffer));
			}
			if(bufferpos < 9)
			{
				buffer[bufferpos] = '.';
				bufferpos++;
			}
			tft_fillRect(500,100,150,50,RA8875_YELLOW);
			tft_textMode();
			tft_textColor(RA8875_BLACK,RA8875_YELLOW);
			tft_textWrite(500,100,140,0, buffer, ALINE_LEFT);
			Action1 = 0;
		}
		if(Action1 == 5)	//	  Minus
		{
			if(first)
			{
				first = 0;
				bufferpos = 0;
				memset(buffer,0,sizeof(buffer));
			}
			if(bufferpos < 9)
			{
				buffer[bufferpos] = '-';
				bufferpos++;
			}
			tft_fillRect(500,100,150,50,RA8875_YELLOW);
			tft_textMode();
			tft_textColor(RA8875_BLACK,RA8875_YELLOW);
			tft_textWrite(500,100,140,0, buffer, ALINE_LEFT);
			Action1 = 0;
		}
		if(Action1 == 2)	//	Cancel
		{
			*result = 0;
			return defval;
		}
		if(Action1 == 1)	//	OK
		{
			float val=atof(buffer);
			if((val < min) || (val > max))
			{
				beep(100);
				Action1 = 0;
			} else 
			{
				*result = 1;	
				return val;
			}
		}
	}
}
int tft_GetInt(char *Text,int defval, int min, int max)
{
	char buffer[100],first=1;
	int bufferpos;
	tft_delete_all_objects();
	tft_fillScreen(RA8875_BLACK);
	tft_drawcalcbuttons();	//	Vykreslim klavesnicu 0-9,....
	if(min < 0)	//	Pouzitelne su aj zaporne cisla
		tft_button(CalcX(10,4,4),CalcY(10,4,1),CalcHeight(10,4),CalcHeight(10,4),"-",5,RA8875_LIME,10);
	tft_textMode();
	tft_textEnlarge(1);
	tft_textColor(RA8875_YELLOW,RA8875_BLACK);
	tft_textWrite(500,10,0,0, (char*)Text, ALINE_LEFT);
	tft_graphicsMode();
	tft_fillRect(500,100,150,50,RA8875_YELLOW);
	bufferpos = sprintf(buffer,"%d",defval);
	tft_textColor(RA8875_BLACK,RA8875_YELLOW);
	tft_textMode();
	tft_textWrite(500,100,140,0, buffer, ALINE_LEFT);
	Action1 = 0;
	while(1)
	{
		tft_draw_press_unpress();
		//		FontNo=FONT_DIGI;
		if(Action1 == 3)	//	Pressed backspace
		{
			if(bufferpos > 0)
			{
				bufferpos--;
				buffer[bufferpos] = 0;
			}
			tft_fillRect(500,100,150,50,RA8875_YELLOW);
			tft_textMode();
			tft_textColor(RA8875_BLACK,RA8875_YELLOW);
			tft_textWrite(500,100,140,0, buffer, ALINE_LEFT);
			Action1 = 0;
		}
		if(Action1 >= 10)	//	Pressed number 0-9
		{
			if(first)
			{
				first = 0;
				bufferpos = 0;
				memset(buffer,0,sizeof(buffer));
			}
			if(bufferpos < 9)
			{
				buffer[bufferpos] = 48 + Action - 10;
				bufferpos++;
			}
			tft_fillRect(500,100,150,50,RA8875_YELLOW);
			tft_textMode();
			tft_textColor(RA8875_BLACK,RA8875_YELLOW);
			tft_textWrite(500,100,140,0, buffer, ALINE_LEFT);
			Action1 = 0;
		}
		if(Action1 == 4)	//	  ciarka
		{
			beep(400);
			Action1 = 0;
		}
		if(Action1 == 5)	//	  Minus
		{
			if(first)
			{
				first = 0;
				bufferpos = 0;
				memset(buffer,0,sizeof(buffer));
			}
			if(bufferpos < 9)
			{
				buffer[bufferpos] = '-';
				bufferpos++;
			}
			tft_fillRect(500,100,150,50,RA8875_YELLOW);
			tft_textMode();
			tft_textColor(RA8875_BLACK,RA8875_YELLOW);
			tft_textWrite(500,100,140,0, buffer, ALINE_LEFT);
			Action1 = 0;
		}
		if(Action1 == 2)	//	Cancel
		{
			return defval;
		}
		if(Action1 == 1)	//	OK
		{
			int val=atol(buffer);
			if((val < min) || (val > max))
			{
				beep(100);
				Action1 = 0;
			} else return val;
		}
	}
}


void TC0_Handler(void)
{
	volatile uint32_t ul_status;
	ul_status = TC0->TC_CHANNEL->TC_SR;	//	vynulujem priznak prerusenie
	if(ul_status)
		needtouch = true;
//	live_counter++;
//	tft_JoyPWMset();
//	need_joystick = true;
	
}
void tft_GetTouch()
{
	if(!needtouch) return;
	uint16_t x,y;
	POINT lcd, touch;
	needtouch = false;
	if(tft_istouched())
	{
		tft_touchRead(&x,&y);
		if (Calibrating) {
			TouchXPos = x;
			TouchYPos = y;
		} else {
			touch.x = x;
			touch.y = y;
			getDisplayPoint(&lcd, &touch, &TouchCalibration);
			TouchXPos = lcd.x;
			TouchYPos = lcd.y;
		}
	}else if(dotyk)	//	Nie je stlacene, ale dotyk uz bol
	{
		unpressed = 1;
		dotyk = 0;
	}
}


void tft_InitTimer()
{
	PMC->PMC_PCER0 |= 1 << ID_TC0;
	
	// Disable TC clock
	TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKDIS;
	
	// Disable interrupts
	TC0->TC_CHANNEL[0].TC_IDR = 0xFFFFFFFF;
	
	// Clear status register
	TC0->TC_CHANNEL[0].TC_SR;
	
	// Set Mode
	TC0->TC_CHANNEL[0].TC_CMR = TC_CMR_CPCTRG | TC_CMR_TCCLKS_TIMER_CLOCK5;
	
	// Compare Value
	TC0->TC_CHANNEL[0].TC_RC = 1000; // 24 = 1 ms; 59 = 2 ms
	
	// Configure and enable interrupt on RC compare
	NVIC_EnableIRQ(TC0_IRQn);
	NVIC_SetPriority(TC0_IRQn, 11);	//	Toto prerusenie musi mat nizsiu prioritu ako prerusenie pre casovanie motorov

	TC0->TC_CHANNEL[0].TC_IER = TC_IER_CPCS;
	
	// Reset counter (SWTRG) and enable counter clock (CLKEN)
	TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
}

void tft_calibrate_load(void)
{
	uint8_t buffer[28];
	for(int i = 0; i< 28; i++)
		buffer[i] = flash_read_8(4 + i);
	memcpy(&TouchCalibration,buffer,28);
}


void tft_calibrate_touch(void) 
{
	int no;
	POINT screenSample[3];   //array of input points
	POINT displaySample[3] = { { 30, 30 }, { 770, 240 }, { 400, 450 } }; //array of expected correct answers  TS.getMatrix();
	Calibrating = 1;
	tft_delete_all_objects();
	tft_fillScreen(RA8875_LIME);
//	delay(500);
	for (no = 0; no < 3; no++) {
		tft_fillScreen(RA8875_LIME);
		tft_fillRectRound(displaySample[no].x - 10, displaySample[no].y - 10,20,20, RA8875_RED,10);
		TouchXPos = -1234;
		while (TouchXPos == -1234)	tft_GetTouch();	//	Pockam na stlacenie touch
		screenSample[no].x = TouchXPos;
		screenSample[no].y = TouchYPos;
//		NVIC_DisableIRQ((IRQn_Type) ID_TC0);
		beep(10);
		tft_fillScreen(RA8875_LIME);
		delay_ms(200);
#if CAPACITIVE_TOUCH != 1
		needtouch = 1;
		tft_GetTouch();
		for(int i = 0;i < 10;i++)
		{
			needtouch = 1;
			tft_GetTouch();
		}
#endif
//		NVIC_EnableIRQ((IRQn_Type) ID_TC0);
	}
	if (setCalibrationMatrix(&displaySample[0], &screenSample[0],&TouchCalibration))
		beep(10);
	Calibrating = 0;
	uint8_t buffer[28];
	memcpy(&buffer,&TouchCalibration,28);
	flash_write_buffer(4,buffer,28);
}


void beep(int ms)
{
	PMC->PMC_PCER0 |= 1 << ID_PIOA;	//	Beep existuje este ako samostatna procedura
	PIOA->PIO_OER |= PIO_PA16;
	PIOA->PIO_SODR = PIO_PA16;
	delay_ms(ms);
	PIOA->PIO_CODR = PIO_PA16;
}

int setCalibrationMatrix( POINT * displayPtr,POINT * screenPtr,TOUCH_MATRIX * matrixPtr)
{
	int  retValue = 0 ;
	matrixPtr->Divider = ((screenPtr[0].x - screenPtr[2].x) * (screenPtr[1].y - screenPtr[2].y)) -	((screenPtr[1].x - screenPtr[2].x) * (screenPtr[0].y - screenPtr[2].y)) ;
	if ( matrixPtr->Divider == 0 )
	{
		retValue = -1 ;
	}
	else
	{
		matrixPtr->An = ((displayPtr[0].x - displayPtr[2].x) * (screenPtr[1].y - screenPtr[2].y)) -
		((displayPtr[1].x - displayPtr[2].x) * (screenPtr[0].y - screenPtr[2].y)) ;

		matrixPtr->Bn = ((screenPtr[0].x - screenPtr[2].x) * (displayPtr[1].x - displayPtr[2].x)) -
		((displayPtr[0].x - displayPtr[2].x) * (screenPtr[1].x - screenPtr[2].x)) ;

		matrixPtr->Cn = (screenPtr[2].x * displayPtr[1].x - screenPtr[1].x * displayPtr[2].x) * screenPtr[0].y +
		(screenPtr[0].x * displayPtr[2].x - screenPtr[2].x * displayPtr[0].x) * screenPtr[1].y +
		(screenPtr[1].x * displayPtr[0].x - screenPtr[0].x * displayPtr[1].x) * screenPtr[2].y ;

		matrixPtr->Dn = ((displayPtr[0].y - displayPtr[2].y) * (screenPtr[1].y - screenPtr[2].y)) -
		((displayPtr[1].y - displayPtr[2].y) * (screenPtr[0].y - screenPtr[2].y)) ;

		matrixPtr->En = ((screenPtr[0].x - screenPtr[2].x) * (displayPtr[1].y - displayPtr[2].y)) -
		((displayPtr[0].y - displayPtr[2].y) * (screenPtr[1].x - screenPtr[2].x)) ;

		matrixPtr->Fn = (screenPtr[2].x * displayPtr[1].y - screenPtr[1].x * displayPtr[2].y) * screenPtr[0].y +
		(screenPtr[0].x * displayPtr[2].y - screenPtr[2].x * displayPtr[0].y) * screenPtr[1].y +
		(screenPtr[1].x * displayPtr[0].y - screenPtr[0].x * displayPtr[1].y) * screenPtr[2].y ;
	}

	return( retValue ) ;

} /* end of setCalibrationMatrix() */

int getDisplayPoint( POINT * displayPtr,POINT * screenPtr,TOUCH_MATRIX * matrixPtr )
{
	int  retValue = 0 ;
	if ( matrixPtr->Divider != 0 )
	{
		/* Operation order is important since we are doing integer */
		/*  math. Make sure you add all terms together before      */
		/*  dividing, so that the remainder is not rounded off     */
		/*  prematurely.                                           */
		displayPtr->x = ( (matrixPtr->An * screenPtr->x) +
		(matrixPtr->Bn * screenPtr->y) +
		matrixPtr->Cn
		) / matrixPtr->Divider ;

		displayPtr->y = ( (matrixPtr->Dn * screenPtr->x) +
		(matrixPtr->En * screenPtr->y) +
		matrixPtr->Fn
		) / matrixPtr->Divider ;
	}
	else
	{
		retValue = -1 ;
	}
	return( retValue ) ;
} /* end of getDisplayPoint() */

