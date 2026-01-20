#include "main.h"

#define VERTICAL 0
#define HORIZONTAL 1
#define LCD_ALIGNMENT VERTICAL
//#define LCD_ALIGNMENT HORIZONTAL

#if LCD_ALIGNMENT == VERTICAL
	#define X_RES 240
	#define Y_RES 320
#else
	#define X_RES 320
	#define Y_RES 240
#endif

#define LCD_CS 11 //GPIO11
#define LCD_DC 12 //GPIO12
#define LCD_RESET 13 //GPIO13

extern SPI_HandleTypeDef hspi1;

void LCD_WriteComm(unsigned char comm);
void LCD_WriteData(unsigned char data);
void LCD_WriteData2(unsigned short data);
void LCD_WriteDataN(unsigned char *b,int n);
void LCD_Init(void);
void LCD_SetCursor(unsigned short x, unsigned short y);
void LCD_Clear(unsigned short color);
void drawPixel(unsigned short x, unsigned short y, unsigned short color);
unsigned short getColor(unsigned short x, unsigned short y);
void set_palette(unsigned char n,unsigned char b,unsigned char r,unsigned char g);
void pset(int x,int y,unsigned char c);
void putbmpmn(int x,int y,unsigned char m,unsigned char n,const unsigned char bmp[]);
void clrbmpmn(int x,int y,unsigned char m,unsigned char n);
void gline(int x1,int y1,int x2,int y2,unsigned char c);
void hline(int x1,int x2,int y,unsigned char c);
void circle(int x0,int y0,unsigned int r,unsigned char c);
void boxfill(int x1,int y1,int x2,int y2,unsigned char c);
void circlefill(int x0,int y0,unsigned int r,unsigned char c);
void putfont(int x,int y,unsigned char c,int bc,unsigned char n);
void printstr(int x,int y,unsigned char c,int bc,unsigned char *s);
void printnum(int x,int y,unsigned char c,int bc,unsigned int n);
void printnum2(int x,int y,unsigned char c,int bc,unsigned int n,unsigned char e);
void init_graphic(void);
extern unsigned short palette[];


