/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ili9341_spi.h"
#include "image.h"


//#define GPIO_KEYUP 0
//#define GPIO_KEYLEFT 1
//#define GPIO_KEYRIGHT 2
//#define GPIO_KEYDOWN 3
//#define GPIO_KEYSTART 4
//#define GPIO_KEYFIRE 5
//#define KEYUP !HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_15)
//#define KEYLEFT !HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_12)
//#define KEYRIGHT !HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_11)
//#define KEYDOWN !HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_10)
//#define KEYSTART !HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_8)
//#define KEYFIRE !HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_9)
//#define KEYSMASK (KEYUP|KEYLEFT|KEYRIGHT|KEYDOWN|KEYSTART|KEYFIRE)

// Joystick thresholds (12-bit ADC: 0-4095, center ~2048)
#define JOYSTICK_CENTER_MIN 1500
#define JOYSTICK_CENTER_MAX 2600
#define JOYSTICK_THRESHOLD 800  // Dead zone around center

// Joystick reading functions
uint16_t joystick_x = 2048;
uint16_t joystick_y = 2048;

void ReadJoystick(void) {
	uint32_t adc_value;
	ADC_ChannelConfTypeDef sConfig = {0};
	
	// Read X axis (PA0 - ADC1_IN0)
	sConfig.Channel = ADC_CHANNEL_0;
	sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 10);
	adc_value = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
	joystick_x = (uint16_t)adc_value;
	
	// Read Y axis (PA1 - ADC1_IN1)
	sConfig.Channel = ADC_CHANNEL_1;
	sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 10);
	adc_value = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
	joystick_y = (uint16_t)adc_value;
}

// Joystick direction macros
#define KEYUP (joystick_y < (JOYSTICK_CENTER_MIN - JOYSTICK_THRESHOLD))
#define KEYDOWN (joystick_y > (JOYSTICK_CENTER_MAX + JOYSTICK_THRESHOLD))
#define KEYLEFT (joystick_x < (JOYSTICK_CENTER_MIN - JOYSTICK_THRESHOLD))
#define KEYRIGHT (joystick_x > (JOYSTICK_CENTER_MAX + JOYSTICK_THRESHOLD))
#define KEYSTART !HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_8)  // Keep START button or use joystick button
#define KEYFIRE !HAL_GPIO_ReadPin(JOYSTICK_SW_GPIO_Port,JOYSTICK_SW_Pin)  // Joystick button
//#define KEYSMASK (KEYUP|KEYLEFT|KEYRIGHT|KEYDOWN|KEYSTART|KEYFIRE)

#define SOUNDPORT 6

#define clearscreen() LCD_Clear(0)

_Character pacman,akabei,pinky,aosuke,guzuta; //Structure for each character
_Music music; //musical structure in play
unsigned int score,highscore; //score, high score
unsigned char player; //Pacman remaining
unsigned char stage; //Current number of stages
unsigned char gamecount; //whole counter
unsigned char gamestatus; //0: Game start, 1: Game in progress, 2: Player decreased by 1, 3: Stage cleared, 4: Game over
unsigned short pacmanspeed,monsterspeed,ijikespeed,medamaspeed;//Movement speed of each character in the current stage (counter)
unsigned short ijiketime; //Ijike time of the current stage
unsigned char fruitno; //Fruit number of current stage
unsigned char upflag; //Check for more players with over 10,000 points
unsigned char fruitflag1,fruitflag2; //Flag indicating whether fruit was served at the current stage
unsigned short fruitcount; //fruit display time counter
unsigned char cookie; //Number of food remaining
unsigned char huntedmonster; //Number of Ijike caught (for score calculation)
unsigned short monsterhuntedtimer; //Stop counter while catching Ijike
unsigned short fruitscoretimer; //Score display counter when acquiring fruits
unsigned char monstersoundcount; //Monster sound effect duration
unsigned char cookiesoundcount,fruitsoundcount,over10000soundcount;//Duration of various sound effects
unsigned short monstersound,monstersounddif; //Monster sound effect value and increase/decrease value (varies depending on mode)
unsigned short monsterhuntedsound; //Ijike capture sound effect value
unsigned short fruitsound; //fruit acquisition sound effect
unsigned short firekeyold; //Pause key state
unsigned char map[MAPXSIZE*MAPYSIZE]; // Indicates that there are passages, walls, food, power food, fruit, and doors.
unsigned char fruit[]={0,1,2,2,3,3,4,4,5,5,6,6,7}; //Fruit number per side
unsigned short fruitscore[]={10,30,50,70,100,200,300,500}; //fruit score
unsigned short pacmansp[]= {135,150,150,150,160,160,160,170,170,170,190,190,190,210,210,210,210,256,256,256,256}; //Pacman speed per face
unsigned short monstersp[]={135,150,150,150,160,160,160,170,170,170,190,190,190,210,210,210,210,256,256,256,256}; //Monster speed per side
unsigned short ijikesp[]=  { 50, 60, 60, 60, 65, 65, 65, 70, 70, 70, 75, 75, 75, 80, 80, 80, 80, 90, 90, 90, 90}; //Ijike speed per surface
unsigned short ijike[]=    {550,520,450,350,130,300,130,120, 60,300,100, 60, 60,180, 60, 60,  0 ,60,  0,  0,  0}; //Ijike time for each side
unsigned short medamasp[]= {512,512,512,512,512,512,512,512,512,512,512,512,512,512,512,512,512,512,512,512,512}; //Eyeball speed per surface

//sounddata Array C ~ C above ~ Period counter value of C above
// 31250/(440*power(2,k/12))*16  k is the difference from A, the lower the pitch, the more negative it becomes.
unsigned short sounddata[]={1911,1804,1703,1607,1517,1432,1351,1276,1204,1136,1073,1012,
						956,902,851,804,758,716,676,638,602,568,536,506,478};
//musicdata array scale, length, scale, length,...Finally, the song ends at scale 254
// Scale 0: C ~ 12: C above ~ 24: C above 255: Rest
// Length: n 60ths of a second
unsigned char musicdata1[]={0,3,12,5,19,8,16,8,0,3,12,8,0,3,19,5,16,8,12,8,255,8,
						1,3,13,5,20,8,17,8,1,3,13,8,1,3,20,5,17,8,13,8,255,8,
						0,3,12,5,19,8,16,8,0,3,12,8,0,3,19,5,16,8,12,8,255,8,
						7,3,17,5,18,8,9,3,19,5,20,8,11,3,21,5,23,8,12,3,24,5,254};
unsigned char musicdata2[]={6,6,255,2,6,6,255,2,6,8,3,4,1,4,6,2,255,2,6,12,10,16,
						6,6,255,2,6,6,255,2,6,8,3,4,1,4,6,2,255,2,6,12,3,16,
						6,6,255,2,6,6,255,2,6,8,3,4,1,4,6,2,255,2,6,12,9,8,11,8,
						12,8,11,8,9,8,6,8,9,8,6,16,255,8,
						6,6,255,2,6,6,255,2,6,8,3,4,1,4,6,2,255,2,6,12,10,16,
						6,6,255,2,6,6,255,2,6,8,3,4,1,4,6,2,255,2,6,12,3,16,
						6,6,255,2,6,6,255,2,6,8,3,4,1,4,6,2,255,2,6,12,9,8,11,8,
						12,8,11,8,9,8,6,8,9,8,6,16,254};
//
//map definition
unsigned char scenedata[MAPXSIZE*MAPYSIZE]={
	//Wall and food arrangement 0x80 to 0x8e: Wall, 0x8f: Nothing (passage), 0x90: Food, 0x91: Power food
	0x80,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x8a,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x81,
	0x85,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x85,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x85,
	0x85,0x90,0x80,0x84,0x81,0x90,0x80,0x84,0x81,0x90,0x85,0x90,0x80,0x84,0x81,0x90,0x80,0x84,0x81,0x90,0x85,
	0x85,0x91,0x85,0x8f,0x85,0x90,0x85,0x8f,0x85,0x90,0x85,0x90,0x85,0x8f,0x85,0x90,0x85,0x8f,0x85,0x91,0x85,
	0x85,0x90,0x82,0x84,0x83,0x90,0x82,0x84,0x83,0x90,0x86,0x90,0x82,0x84,0x83,0x90,0x82,0x84,0x83,0x90,0x85,
	0x85,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x85,
	0x85,0x90,0x80,0x84,0x81,0x90,0x87,0x90,0x80,0x84,0x84,0x84,0x81,0x90,0x87,0x90,0x80,0x84,0x81,0x90,0x85,
	0x85,0x90,0x82,0x84,0x83,0x90,0x85,0x90,0x82,0x84,0x8a,0x84,0x83,0x90,0x85,0x90,0x82,0x84,0x83,0x90,0x85,
	0x85,0x90,0x90,0x90,0x90,0x90,0x85,0x90,0x90,0x90,0x85,0x90,0x90,0x90,0x85,0x90,0x90,0x90,0x90,0x90,0x85,
	0x82,0x84,0x84,0x84,0x81,0x90,0x8d,0x84,0x89,0x90,0x86,0x90,0x88,0x84,0x8c,0x90,0x80,0x84,0x84,0x84,0x83,
	0x8f,0x8f,0x8f,0x8f,0x85,0x90,0x85,0x8f,0x8f,0x8f,0x8f,0x8f,0x8f,0x8f,0x85,0x90,0x85,0x8f,0x8f,0x8f,0x8f,
	0x8f,0x8f,0x8f,0x8f,0x85,0x90,0x85,0x8f,0x80,0x84,0x8e,0x84,0x81,0x8f,0x85,0x90,0x85,0x8f,0x8f,0x8f,0x8f,
	0x88,0x84,0x84,0x84,0x83,0x90,0x86,0x8f,0x85,0x8f,0x8f,0x8f,0x85,0x8f,0x86,0x90,0x82,0x84,0x84,0x84,0x89,
	0x8f,0x8f,0x8f,0x8f,0x8f,0x90,0x8f,0x8f,0x85,0x8f,0x8f,0x8f,0x85,0x8f,0x8f,0x90,0x8f,0x8f,0x8f,0x8f,0x8f,
	0x88,0x84,0x84,0x84,0x81,0x90,0x87,0x8f,0x82,0x84,0x84,0x84,0x83,0x8f,0x87,0x90,0x80,0x84,0x84,0x84,0x89,
	0x8f,0x8f,0x8f,0x8f,0x85,0x90,0x85,0x8f,0x8f,0x8f,0x8f,0x8f,0x8f,0x8f,0x85,0x90,0x85,0x8f,0x8f,0x8f,0x8f,
	0x8f,0x8f,0x8f,0x8f,0x85,0x90,0x85,0x8f,0x80,0x84,0x84,0x84,0x81,0x8f,0x85,0x90,0x85,0x8f,0x8f,0x8f,0x8f,
	0x80,0x84,0x84,0x84,0x83,0x90,0x86,0x8f,0x82,0x84,0x8a,0x84,0x83,0x8f,0x86,0x90,0x82,0x84,0x84,0x84,0x81,
	0x85,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x85,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x85,
	0x85,0x90,0x88,0x84,0x81,0x90,0x88,0x84,0x89,0x90,0x86,0x90,0x88,0x84,0x89,0x90,0x80,0x84,0x89,0x90,0x85,
	0x85,0x91,0x90,0x90,0x85,0x90,0x90,0x90,0x90,0x90,0x8f,0x90,0x90,0x90,0x90,0x90,0x85,0x90,0x90,0x91,0x85,
	0x8d,0x84,0x81,0x90,0x85,0x90,0x87,0x90,0x80,0x84,0x84,0x84,0x81,0x90,0x87,0x90,0x85,0x90,0x80,0x84,0x8c,
	0x8d,0x84,0x83,0x90,0x86,0x90,0x85,0x90,0x82,0x84,0x8a,0x84,0x83,0x90,0x85,0x90,0x86,0x90,0x82,0x84,0x8c,
	0x85,0x90,0x90,0x90,0x90,0x90,0x85,0x90,0x90,0x90,0x85,0x90,0x90,0x90,0x85,0x90,0x90,0x90,0x90,0x90,0x85,
	0x85,0x90,0x88,0x84,0x84,0x84,0x8b,0x84,0x89,0x90,0x86,0x90,0x88,0x84,0x8b,0x84,0x84,0x84,0x89,0x90,0x85,
	0x85,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x85,
	0x82,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x83
};

//Macro settings Reading and writing of virtual map
#define GETMAP(x,y) map[(y)*MAPXSIZE+(x)]
#define SETMAP(x,y,d) map[(y)*MAPXSIZE+(x)]=(d)

#define PWM_WRAP 4000 // 125MHz/31.25KHz
uint16_t pwm_slice_num;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
ADC_HandleTypeDef hadc1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void sound_on(uint16_t f){
//	pwm_set_clkdiv_int_frac(pwm_slice_num, f>>4, f&15);
//	pwm_set_enabled(pwm_slice_num, true);
}
void sound_off(void){
//	pwm_set_enabled(pwm_slice_num, false);
}

void wait60thsec(unsigned short n){
	// Wait for n seconds of 60 minutes
	uint64_t t=33;
	DelayUs(166*n-t);
}
unsigned char startkeycheck(unsigned short n){
	// Wait for n seconds of 60 minutes
	// Return immediately if the start button is pressed
	//Return value 1 if the start button is pressed, 0 if not
//	uint64_t t=to_us_since_boot(get_absolute_time())%16667;
	uint64_t t=33;
	while(n--){
		DelayUs(16667-t);
		if(KEYSTART){
			return 1;
		}
		t=0;
	}
	return 0;
}
void playmusic60thsec(void){
	//After waiting for 1/60th of a second, advance the currently playing song by one.
	wait60thsec(1); //1/60th second wait
	if(music.stop) return; //Performance finished
	music.count--;
	if(music.count>0) return;

	//play the next note
	if(*music.p==254){ //Song ends
		music.stop=1;
		sound_off();
		return;
	}
	if(*music.p==255) sound_off(); //rest
	else sound_on(sounddata[*music.p]); //period data
	music.p++;
	music.count=*music.p; //note length
	music.p++;
}
void startmusic(unsigned char *m){
	music.p=m;
	music.count=1;
	music.stop=0;
}
/*
//Simply playing a song (not used this time)
void playmusic(unsigned char *m){
	startmusic(m);
	while(music.stop==0){
		playmusic60thsec();
	}
}
*/
void printchar(unsigned char x,unsigned char y,unsigned char c,unsigned char n){
	//Display one character of text code n with color number c at character coordinates (x,y)
	putfont(x*8,y*8,c,0,n);
}
void printstrc(unsigned char x,unsigned char y,unsigned char c,unsigned char *s){
	//Display string s with color number c from coordinates (x,y)
	while(*s){
		printchar(x++,y,c,*s++);
	}
}
void printscore(unsigned char x,unsigned char y,unsigned char c,unsigned int s){
	//Display score s with color number c at coordinates (x, y) (5 digits)
	x+=5;
	do{
		printchar(x,y,c,'0'+s%10);
		s/=10;
		x--;
	}while(s!=0);
}

void putbmpmn3(int x,int y,unsigned char m,unsigned char n,const unsigned char bmp[])
//Display a character of width m*vertical n dots at coordinates x,y
//Click here if you want to draw only within the game map (parts that protrude outside the map will not be displayed)
// Simply arrange the color numbers in the unsigned char bmp[m*n] array
// Treat the part with color number 0 as transparent color
{
	int i,j;
	int skip;
	static uint8_t lcddatabuf[64];
	uint8_t *lcdbufp;
	const unsigned char *p;
	if(x<=-m || x>=MAPXSIZE*8 || y<=-n || y>=MAPYSIZE*8) return; //off screen
	if(y<0){ //If the screen is cut off at the top
		i=0;
		p=bmp-y*m;
	}
	else{
		i=y;
		p=bmp;
	}
	for(;i<y+n;i++){
		if(i>=MAPYSIZE*8) return; //If the screen is cut off at the bottom
		if(x<0){ //If the screen cuts to the left, only the remaining part is drawn.
			j=0;
			p+=-x;
		}
		else{
			j=x;
		}
		skip=1;
		lcdbufp=lcddatabuf;
		for(;j<x+m;j++){
			if(j>=MAPXSIZE*8){ //If the screen cuts to the right
				p+=x+m-j;
				break;
			}
			if(*p!=0){ //If the color number is 0, it is treated as transparent.
				if(skip){
					LCD_SetCursor(j,i);
					skip=0;
				}
				*lcdbufp++=palette[*p]>>8;
				*lcdbufp++=(uint8_t)palette[*p];
			}
			else{
				skip=1;
				if(lcdbufp!=lcddatabuf){
					LCD_WriteDataN(lcddatabuf,lcdbufp-lcddatabuf);
					lcdbufp=lcddatabuf;
				}
			}
			p++;
		}
		if(lcdbufp!=lcddatabuf){
			LCD_WriteDataN(lcddatabuf,lcdbufp-lcddatabuf);
		}
	}
}
void putpacman(void){
	//Pacman display
	unsigned char a;
	pacman.animcount--;
	if(pacman.animcount==0){
		pacman.animcount=pacman.animcount0;
		pacman.animvalue++;
		if(pacman.animvalue==6) pacman.animvalue=0;
	}
	if(pacman.animvalue==0) a=0;
	else if(pacman.animvalue<=3) a=pacman.dir*3+pacman.animvalue;
	else a=pacman.dir*3+6-pacman.animvalue;
	putbmpmn3((int)(pacman.x/256)-3,(int)(pacman.y/256)-3,XWIDTH_PACMAN,YWIDTH_PACMAN,Pacmanbmp[a]);
}
void putmonster(_Character *p){
	//Monster display p: Character pointer specification
	unsigned char i;//Monster foot patterns (2 types)
	if(gamecount & 4) i=0;
	else i=1;
	switch(p->status){
		case IJIKE:
			if(p->modecount>180 || gamecount & 8)
				putbmpmn3((int)(p->x/256)-3,(int)(p->y/256)-3,XWIDTH_MONSTER,YWIDTH_MONSTER,Ijikebmp[i]);
			else
				//flashing white
				putbmpmn3((int)(p->x/256)-3,(int)(p->y/256)-3,XWIDTH_MONSTER,YWIDTH_MONSTER,Ijikebmp[2+i]);
			break;
		case MEDAMA:
			putbmpmn3((int)(p->x/256)-3,(int)(p->y/256)-3,XWIDTH_MONSTER,YWIDTH_MONSTER,Medamabmp[p->dir]);
			break;
		default:
			putbmpmn3((int)(p->x/256)-3,(int)(p->y/256)-3,XWIDTH_MONSTER,YWIDTH_MONSTER,Monsterbmp[p->no*8+p->dir*2+i]);
	}
}
void blinkpowercookie(){
	//power bait flashing
	if(gamecount & 0x10){
		set_palette(COLOR_POWERCOOKIE,0,50,50);
	}
	else{
		set_palette(COLOR_POWERCOOKIE,0,255,255);
	}
}
unsigned char getfruitno(unsigned char s){
	// Returns the fruit number of the s side
	if(s<13) return fruit[s-1];
	else return fruit[12];
}
void putfruit(void){
	// fruit display
	putbmpmn3(FRUITX*8-2,FRUITY*8-3,12,14,Fruitbmp[fruitno]);
}
void putmapchar(unsigned char x,unsigned char y){
	//Show things according to the code on the map
	unsigned char d;

	d=GETMAP(x,y);
	if(d==MAP_COOKIE) printchar(x,y,COLOR_COOKIE,CODE_COOKIE);
	else if(d==MAP_POWERCOOKIE) printchar(x,y,COLOR_POWERCOOKIE,CODE_POWERCOOKIE);
	else if(d==MAP_DOOR) printchar(x,y,COLOR_DOOR,CODE_DOOR);
	else if(d==MAP_WALL) printchar(x,y,COLOR_WALL,scenedata[y*MAPXSIZE+x]);
	else printchar(x,y,0,' ');
}
void setfruit(unsigned char f){
	// f 0: Fruit deletion & display erase, 1: Fruit generation & display
	if(f){
		SETMAP(FRUITX,FRUITY,MAP_FRUIT);
	}
	else{
		SETMAP(FRUITX,FRUITY,MAP_NONE);
		putmapchar(FRUITX-1,FRUITY-1);
		putmapchar(FRUITX  ,FRUITY-1);
		putmapchar(FRUITX+1,FRUITY-1);
		putmapchar(FRUITX-1,FRUITY  );
		putmapchar(FRUITX  ,FRUITY  );
		putmapchar(FRUITX+1,FRUITY  );
		putmapchar(FRUITX-1,FRUITY+1);
		putmapchar(FRUITX  ,FRUITY+1);
		putmapchar(FRUITX+1,FRUITY+1);
	}
}
void getfruit(){
	// ate fruit

	score+=fruitscore[fruitno];
	fruitcount=0;
	setfruit(0);
	fruitsound=22000;
	fruitsoundcount=20;
	fruitscoretimer=TIMER_FRUITSCORE;
}

void displayscore(){
	if(score>highscore) highscore=score;
	printscore(21,3,7,highscore);
	printscore(21,7,7,score);
}
void displayplayers(){
	//Display of remaining number of players
	unsigned char i;
	for(i=0;i<player && i<5;i++) putbmpmn(22*8+i*16,23*8,XWIDTH_PACMAN,YWIDTH_PACMAN,Pacmanbmp[11]);
	for(;i<4;i++) clrbmpmn(22*8+i*16,23*8,XWIDTH_PACMAN,YWIDTH_PACMAN);
}
void displayfruits(){
	//Display fruits for 8 stages on the right side of the screen
	unsigned char i,no;
	for(i=0;i<8 && i<stage;i++){
		no=getfruitno(stage-i);
		clrbmpmn (22*8+(i%4)*16,15*8+(i/4)*16,12,14);
		putbmpmn(22*8+(i%4)*16,15*8+(i/4)*16,12,14,Fruitbmp[no]);
	}
}
void putmap(void){
	//Passage, bait, power bait display
	unsigned char *p,*mapp;
	int i,j;
	clearscreen();
	p=scenedata;
	mapp=map;
	cookie=0;
	for(i=0;i<MAPYSIZE;i++){
		for(j=0;j<MAPXSIZE;j++){
			if(*p<=0x8d){
				*mapp=MAP_WALL; //WALL
				printchar(j,i,COLOR_WALL,*p);
			}
			else if(*p==CODE_DOOR){
				*mapp=MAP_DOOR; //DOOR
				printchar(j,i,COLOR_DOOR,CODE_DOOR);
			}
			else if(*p==CODE_COOKIE){
				*mapp=MAP_COOKIE; //COOKIE
				printchar(j,i,COLOR_COOKIE,CODE_COOKIE);
				cookie++;
			}
			else if(*p==CODE_POWERCOOKIE){
				*mapp=MAP_POWERCOOKIE; //POWERCOOKIE
				printchar(j,i,COLOR_POWERCOOKIE,CODE_POWERCOOKIE);
				cookie++;
			}
			else *mapp=0;
			p++;
			mapp++;
		}
	}
	printstrc(21,1,7,"HI-SCORE");
	printchar(27,3,7,'0');
	printstrc(22,5,7,"1UP");
	printchar(27,7,7,'0');
	displayscore();
	displayplayers();
	displayfruits();
}
void initcharacter(_Character *p,unsigned char s,unsigned short x,unsigned short y,
					unsigned char dir,unsigned short speed,unsigned char ac,unsigned short modec)
{
	//Character initialization
	//p: Character pointer, s: Mode (status), (x,y): Initial position, dir: Initial direction
	//speed: movement speed, ac: animation counter, modec: mode counter
	p->status=s;
	p->x=x;
	p->y=y;
	p->oldx=x;
	p->oldy=y;
	p->dir=dir;
	p->turn=0;
	p->speed=speed;
	p->animcount=ac;
	p->animcount0=ac;
	p->animvalue=0;
	p->modecount=modec;
}
void gameinit(void)
{
	// Initialize the entire game. Only once after startup
	set_palette(COLOR_GUZUTA,70,200,128); //guzta color
	set_palette(COLOR_PINKY,180,230,128); //pinky color
	set_palette(COLOR_WALL,250,20,40); //wall color
	set_palette(COLOR_POWERCOOKIE,0,255,255); //power bait color
	set_palette(COLOR_ORANGE,0,255,165); //orange color
	set_palette(COLOR_MONSTERNUDE,180,245,220); //naked monster color
	akabei.no=AKABEI;
	aosuke.no=AOSUKE;
	guzuta.no=GUZUTA;
	pinky.no=PINKY;
	score=0;
}
void gameinit2(void)
{
	// Game initialization. Call each time with the play button
	score=0;
	player=3;
	upflag=0;//1000 For checking player +1 after crossing the point.
	stage=0;
	firekeyold=0;
}
void gameinit3(void)
{
// Called every time a scene is cleared
	if(stage<STAGEMAX-1){
		ijiketime=ijike[stage];
		pacmanspeed=pacmansp[stage];
		monsterspeed=monstersp[stage];
		ijikespeed=ijikesp[stage];
		medamaspeed=medamasp[stage];
	}
	else{
		ijiketime=ijike[STAGEMAX-1];
		pacmanspeed=pacmansp[STAGEMAX-1];
		monsterspeed=monstersp[STAGEMAX-1];
		ijikespeed=ijikesp[STAGEMAX-1];
		medamaspeed=medamasp[STAGEMAX-1];
	}
	stage++;
	fruitflag1=1;//Fruit from the first half of the surface remains
	fruitflag2=1;//Fruit on the second half of the side remains
	fruitno=getfruitno(stage);
	putmap();//Display of aisles, etc.
}
void gameinit4(void)
{
//Called each time Pac-Man is defeated and each scene is cleared.
	fruitcount=0;
	fruitscoretimer=0;
	huntedmonster=0;
	monsterhuntedtimer=0;
	monstersoundcount=0;
	cookiesoundcount=0;
	fruitsoundcount=0;
	over10000soundcount=0;
	setfruit(0);//Delete fruits, clear display
	initcharacter(&pacman,0,10*8*256,20*8*256,DIR_LEFT,pacmanspeed,5,0);
	initcharacter(&akabei,NAWABARI,MONSTERHOUSEX*8*256,(MONSTERHOUSEY-2)*8*256,DIR_LEFT,monsterspeed,0,550);
	initcharacter(&pinky ,NAWABARI,MONSTERHOUSEX*8*256,(MONSTERHOUSEY+1)*8*256,DIR_UP,monsterspeed,0,0);
	initcharacter(&aosuke,TAIKI2,(MONSTERHOUSEX-1)*8*256,(MONSTERHOUSEY+1)*8*256,DIR_DOWN,monsterspeed,0,375);
	initcharacter(&guzuta,TAIKI2,(MONSTERHOUSEX+1)*8*256,(MONSTERHOUSEY+1)*8*256,DIR_DOWN,monsterspeed,0,600);
}

void keycheck()
{
	//Check the joystick and change Pac-Man's direction if it's not a wall.
	unsigned short k;
	unsigned char d;
	unsigned short x,y;

	// Read joystick position
	ReadJoystick();
	
	x=pacman.x/256;
	y=pacman.y/256;
//	k=~gpio_get_all() & KEYSMASK;
	if(KEYUP && (x%8)==0){	//top direction
		if(pacman.dir!=DIR_UP){
			if(y>=8){
				if(GETMAP(x/8,y/8-1)!=MAP_WALL) pacman.dir=DIR_UP;
			}
			else pacman.dir=DIR_UP;
		}
	}
	else if(KEYRIGHT && (y%8)==0){	//right button
		if(pacman.dir!=DIR_RIGHT){
			if(x<=(MAPXSIZE-2)*8){
				if(GETMAP(x/8+1,y/8)!=MAP_WALL) pacman.dir=DIR_RIGHT;
			}
			else pacman.dir=DIR_RIGHT;
		}
	}
	else if(KEYDOWN && (x%8)==0){	//bottom button
		if(pacman.dir!=DIR_DOWN){
			if(y<=(MAPYSIZE-2)*8){
				d=GETMAP(x/8,y/8+1);
				if(d!=MAP_WALL && d!=MAP_DOOR) pacman.dir=DIR_DOWN;
			}
			else pacman.dir=DIR_DOWN;
		}
	}
	else if(KEYLEFT && (y%8)==0){	//left button
		if(pacman.dir!=DIR_LEFT){
			if(x>=8){
				if(GETMAP(x/8-1,y/8)!=MAP_WALL) pacman.dir=DIR_LEFT;
			}
			else pacman.dir=DIR_LEFT;
		}
	}
}

void movepacman(){
	//Pacman movement check
	unsigned char t;
	unsigned short x,y;

	if(monsterhuntedtimer>0) return; //Stops while capturing monsters
	x=pacman.x/256;
	y=pacman.y/256;
	switch(pacman.dir){
		case DIR_UP: //up
			if(y!=0){
				if((y%8)!=0 || GETMAP(x/8,y/8-1)!=MAP_WALL) pacman.y-=pacman.speed;
			}
			else pacman.y=(MAPYSIZE-1)*8*256;
			break;
		case DIR_RIGHT: //right
			if(x!=(MAPXSIZE-1)*8){
				if((x%8)!=0 || GETMAP(x/8+1,y/8)!=MAP_WALL) pacman.x+=pacman.speed;
			}
			else pacman.x=0;
			break;
		case DIR_DOWN: //under. The monster house door is also considered a wall.
			if(y!=(MAPYSIZE-1)*8){
				t=GETMAP(x/8,y/8+1);
				if((y%8)!=0 || t!=MAP_WALL && t!=MAP_DOOR) pacman.y+=pacman.speed;
			}
			else pacman.y=0;
			break;
		case DIR_LEFT: //left
			if(x!=0){
				if((x%8)!=0 || GETMAP(x/8-1,y/8)!=MAP_WALL) pacman.x-=pacman.speed;
			}
			else pacman.x=(MAPXSIZE-1)*8*256;
	}
}
void movemonster(_Character *p,unsigned short tx,unsigned short ty){
	//Determine the movement direction of the monster and move it
	//p: Monster pointer, tx: Target x coordinate, ty: Target y coordinate
	unsigned char cUP,cRIGHT,cDOWN,cLEFT;
	unsigned char t;
	unsigned char olddir;
	unsigned short x,y,x1,y1,oldx,oldy;

	x=p->x/256;
	y=p->y/256;
	x1=x/8;
	y1=y/8;
	oldx=p->oldx;
	oldy=p->oldy;
	p->oldx=p->x;
	p->oldy=p->y;
	olddir=p->dir;

	switch(p->dir){
	//After changing direction, go straight until you have moved beyond the decimal point
	//Also, if the decimal point is moved while going straight, go straight
		case DIR_UP: //up
			if(p->turn && y==oldy/256){
				p->y-=p->speed;
				return;
			}
			p->turn=0;
			if(y==(unsigned short)((p->y-p->speed))/256){
				p->y-=p->speed;
				return;
			}
			break;
		case DIR_RIGHT: //right
			if(p->turn && x==oldx/256){
				p->x+=p->speed;
				return;
			}
			p->turn=0;
			if(x==(p->x+p->speed)/256){
				p->x+=p->speed;
				return;
			}
			break;
		case DIR_DOWN: //down
			if(p->turn && y==oldy/256){
				p->y+=p->speed;
				return;
			}
			p->turn=0;
			if(y==(p->y+p->speed)/256){
				p->y+=p->speed;
				return;
			}
			break;
		case DIR_LEFT: //left
			if(p->turn && x==oldx/256){
				p->x-=p->speed;
				return;
			}
			p->turn=0;
			if(x==(unsigned short)((p->x-p->speed))/256){
				p->x-=p->speed;
				return;
			}
	}

	//cUP, cRIGHT, cDOWN, cLEFT: Set to 1 if you can pass in each direction, 0 if you cannot pass
	cUP=1;
	cRIGHT=1;
	cDOWN=1;
	cLEFT=1;
	if((x%8)!=0){
		cUP=0;
		cDOWN=0;
	}
	else if((y%8)==0){
		if(y1!=0 && GETMAP(x1,y1-1)==MAP_WALL) cUP=0;
		//One-way traffic checks except when it's a highlight
		else if(p->status!=MEDAMA &&
		  (x1==ONEWAY1X && y1==ONEWAY1Y+1 || x1==ONEWAY2X && y1==ONEWAY2Y+1 ||
		   x1==ONEWAY3X && y1==ONEWAY3Y+1 || x1==ONEWAY4X && y1==ONEWAY4Y+1)) cUP=0;
		if(y1!=MAPYSIZE-1){
			//Treat the door as a wall, except when it's a centerpiece.
			t=GETMAP(x1,y1+1);
			if(t==MAP_WALL || (t==MAP_DOOR && p->status!=MEDAMA)) cDOWN=0;
		}
	}

	if((y%8)!=0){
		cRIGHT=0;
		cLEFT=0;
	}
	else if((x%8)==0){
		if(x1!=MAPXSIZE-1 && GETMAP(x1+1,y1)==MAP_WALL) cRIGHT=0;
		if(x1!=0 && GETMAP(x1-1,y1)==MAP_WALL) cLEFT=0;
	}

	tx>>=8;
	ty>>=8;
	if(p->status==TAIKI2){
		if(y==MONSTERHOUSEY*8) p->dir=DIR_DOWN;
		else if(y==(MONSTERHOUSEY+1)*8) p->dir=DIR_UP;
	}
	else switch(p->dir){
		case DIR_UP: //Proceeding direction=up・・・priority right→up→left
			if(x<tx && cRIGHT) p->dir=DIR_RIGHT;
			else if(y>ty && cUP) p->dir=DIR_UP;
			else if(x>tx && cLEFT) p->dir=DIR_LEFT;
			else if(cRIGHT) p->dir=DIR_RIGHT;
			else if(cUP) p->dir=DIR_UP;
			else p->dir=DIR_LEFT;
			break;
		case DIR_RIGHT: //Proceeding direction=right・・・priority: lower→right→upper
			if(y<ty && cDOWN) p->dir=DIR_DOWN;
			else if(x<tx && cRIGHT) p->dir=DIR_RIGHT;
			else if(y>ty && cUP) p->dir=DIR_UP;
			else if(cDOWN) p->dir=DIR_DOWN;
			else if(cRIGHT) p->dir=DIR_RIGHT;
			else p->dir=DIR_UP;
			break;
		case DIR_DOWN: //Proceeding direction=down・・・priority left→down→right
			if(x>tx && cLEFT) p->dir=DIR_LEFT;
			else if(y<ty && cDOWN) p->dir=DIR_DOWN;
			else if(x<tx && cRIGHT) p->dir=DIR_RIGHT;
			else if(cLEFT) p->dir=DIR_LEFT;
			else if(cDOWN) p->dir=DIR_DOWN;
			else p->dir=DIR_RIGHT;
			break;
		case DIR_LEFT: //Proceeding direction=Left・・・Priority Up→Left→Down
			if(y>ty && cUP) p->dir=DIR_UP;
			else if(x>tx && cLEFT) p->dir=DIR_LEFT;
			else if(y<ty && cDOWN) p->dir=DIR_DOWN;
			else if(cUP) p->dir=DIR_UP;
			else if(cLEFT) p->dir=DIR_LEFT;
			else p->dir=DIR_DOWN;
	}
	if(p->dir != olddir) p->turn=1;
	switch(p->dir){
		case DIR_UP: //up
			if(y!=0) p->y-=p->speed;
			else p->y=(MAPYSIZE-1)*8*256;
			break;
		case DIR_RIGHT: //right
			if(x!=(MAPXSIZE-1)*8){
				//Slow down in the warp zone
				if(y1==WARPY && (x1<WARPX1 || x1>=WARPX2) && p->status!=MEDAMA) p->x+=p->speed/2;
				else p->x+=p->speed;
			}
			else p->x=0;
			break;
		case DIR_DOWN: //down
			if(y!=(MAPYSIZE-1)*8) p->y+=p->speed;
			else p->y=0;
			break;
		case DIR_LEFT: //left
			if(x!=0){
				//Slow down in the warp zone
				if(y1==WARPY && (x1<WARPX1 || x1>=WARPX2) && p->status!=MEDAMA) p->x-=p->speed/2;
				else p->x-=p->speed;
			}
			else p->x=(MAPXSIZE-1)*8*256;
	}
}

void moveakabei(){
	//Akabay move
	unsigned short targetx,targety;

	if(monsterhuntedtimer>0 && akabei.status!=MEDAMA) return; //When someone is captured, all but the eyeballs stop
	switch (akabei.status){
		case NAWABARI:
			akabei.modecount--;
			if(akabei.modecount==0){
				akabei.modecount=TIMER_OIKAKE;
				akabei.status=OIKAKE;
				akabei.dir=(akabei.dir+2)&3; //Reverse direction
				akabei.turn=0;
				//Change other monsters at the same time
				if(pinky.status==NAWABARI){
					pinky.status=OIKAKE;
					pinky.dir=(pinky.dir+2)&3;
					pinky.turn=0;
				}
				if(aosuke.status==NAWABARI){
					aosuke.status=OIKAKE;
					aosuke.dir=(aosuke.dir+2)&3;
					aosuke.turn=0;
				}
				if(guzuta.status==NAWABARI){
					guzuta.status=OIKAKE;
					guzuta.dir=(guzuta.dir+2)&3;
					guzuta.turn=0;
				}
			}
			break;
		case OIKAKE:
			akabei.modecount--;
			if(akabei.modecount==0){
				akabei.modecount=TIMER_NAWABARI;
				akabei.status=NAWABARI;
				akabei.dir=(akabei.dir+2)&3; //Reverse direction
				akabei.turn=0;
				if(pinky.status==OIKAKE){
					pinky.status=NAWABARI;
					pinky.dir=(pinky.dir+2)&3;
					pinky.turn=0;
				}
				if(aosuke.status==OIKAKE){
					aosuke.status=NAWABARI;
					aosuke.dir=(aosuke.dir+2)&3;
					aosuke.turn=0;
				}
				if(guzuta.status==OIKAKE){
					guzuta.status=NAWABARI;
					guzuta.dir=(guzuta.dir+2)&3;
					guzuta.turn=0;
				}
			}
			break;
		case IJIKE:
			akabei.modecount--;
			if(akabei.modecount==0){
				akabei.status=OIKAKE;
				akabei.modecount=TIMER_OIKAKE;
				akabei.speed=monsterspeed;
			}
			break;
		case MEDAMA:
			if((akabei.x/256)==(MONSTERHOUSEX*8) && (akabei.y/256)==(MONSTERHOUSEY*8)){ //Arrival at Monster House
				akabei.status=TAIKI2;
				akabei.modecount=1;
				akabei.dir=DIR_UP;
				akabei.speed=monsterspeed;
			}
			break;
		case TAIKI:
			if((akabei.x/256)==(MONSTERHOUSEX*8) && (akabei.y/256)==((MONSTERHOUSEY-2)*8)){ //I left the monster house
				akabei.status=OIKAKE;
				akabei.modecount=TIMER_OIKAKE;
				akabei.dir=DIR_LEFT;
			}
			break;
		case TAIKI2:
			akabei.modecount--;
			if(akabei.modecount==0){
				akabei.status=TAIKI;
			}
	}
	switch (akabei.status){
		case NAWABARI:
		case IJIKE:
			targetx=NAWABARIAKABEIX*8*256;
			targety=NAWABARIAKABEIY*8*256;
			break;
		case OIKAKE:
			targetx=pacman.x;
			targety=pacman.y;
			break;
		case MEDAMA:
			targetx=MONSTERHOUSEX*8*256;
			targety=MONSTERHOUSEY*8*256;
			break;
		case TAIKI:
			targetx=MONSTERHOUSEX*8*256;
			targety=(MONSTERHOUSEY-2)*8*256;
	}
	movemonster(&akabei,targetx,targety);
}

void movepinky(){
	//pinky move
	unsigned short targetx,targety;

	if(monsterhuntedtimer>0 && pinky.status!=MEDAMA) return; //When someone is captured, everything except the eyeballs stops
	switch (pinky.status){
		case NAWABARI:
		case OIKAKE:
			break;
		case IJIKE:
			pinky.modecount--;
			if(pinky.modecount==0){
				pinky.status=OIKAKE;
				pinky.speed=monsterspeed;
			}
			break;
		case MEDAMA:
			if((pinky.x/256)==(MONSTERHOUSEX*8) && (pinky.y/256)==(MONSTERHOUSEY*8)){ //Arrival at Monster House
				pinky.status=TAIKI2;
				pinky.modecount=1;
				pinky.dir=DIR_UP;
				pinky.speed=monsterspeed;
			}
			else pinky.speed=medamaspeed;
			break;
		case TAIKI:
			if((pinky.x/256)==(MONSTERHOUSEX*8) && (pinky.y/256)==((MONSTERHOUSEY-2)*8)){ //I left the monster house
				pinky.status=OIKAKE;
				pinky.dir=DIR_LEFT;
			}
			break;
		case TAIKI2:
			pinky.modecount--;
			if(pinky.modecount==0){
				pinky.status=TAIKI;
			}
	}
	switch (pinky.status){
		case NAWABARI:
			targetx=NAWABARIPINKYX*8*256;
			targety=NAWABARIPINKYY*8*256;
			break;
		case IJIKE:
			targetx=MONSTERHOUSEX*8*256;
			targety=MONSTERHOUSEY*8*256;
			break;
		case OIKAKE: //Set Pac-Man's direction of movement +3 as the target coordinates.
			if(pacman.dir==DIR_UP){
				targetx=pacman.x;
				if(pacman.y>=4*8*256) targety=pacman.y-3*8*256;
				else if(pacman.y/256/8==0) targety=(MAPYSIZE-3)*8*256;
				else targety=1*8*256;
			}
			else if(pacman.dir==DIR_RIGHT){
				targety=pacman.y;
				if(pacman.x<=(MAPXSIZE-5)*8*256) targetx=pacman.x+3*8*256;
				else if(pacman.x/256/8==(MAPXSIZE-1)*8) targetx=2*8*256;
				else targetx=(MAPXSIZE-2)*8*256;
			}
			else if(pacman.dir==DIR_DOWN){
				targetx=pacman.x;
				if(pacman.y<=(MAPYSIZE-5)*8*256) targety=pacman.y+3*8*256;
				else if(pacman.y/256/8==(MAPYSIZE-1)*8) targety=2*8*256;
				else targety=(MAPYSIZE-2)*8*256;
			}
			else if(pacman.dir==DIR_LEFT){
				targety=pacman.y;
				if(pacman.x>=4*8*256) targetx=pacman.x-3*8*256;
				else if(pacman.x/256/8==0) targetx=(MAPXSIZE-3)*8*256;
				else targetx=1*8*256;
			}
			break;
		case MEDAMA:
			targetx=MONSTERHOUSEX*8*256;
			targety=MONSTERHOUSEY*8*256;
			break;
		case TAIKI:
			targetx=MONSTERHOUSEX*8*256;
			targety=(MONSTERHOUSEY-2)*8*256;
	}
	movemonster(&pinky,targetx,targety);
}

void moveaosuke()
{
	//Aosuke move
	unsigned short targetx,targety;

	if(monsterhuntedtimer>0 && aosuke.status!=MEDAMA) return; //When someone is captured, everything except the eyeballs stops
	switch (aosuke.status){
		case NAWABARI:
		case OIKAKE:
			break;
		case IJIKE:
			aosuke.modecount--;
			if(aosuke.modecount==0){
				aosuke.status=OIKAKE;
				aosuke.speed=monsterspeed;
			}
			break;
		case MEDAMA:
			if((aosuke.x/256)==(MONSTERHOUSEX*8) && (aosuke.y/256)==(MONSTERHOUSEY*8)){ //Arrival at Monster House
				aosuke.status=TAIKI2;
				aosuke.modecount=1;
				aosuke.dir=DIR_UP;
				aosuke.speed=monsterspeed;
			}
			break;
		case TAIKI:
			if((aosuke.x/256)==(MONSTERHOUSEX*8) && (aosuke.y/256)==((MONSTERHOUSEY-2)*8)){ //I left the monster house
				aosuke.status=OIKAKE;
				aosuke.dir=DIR_LEFT;
			}
			break;
		case TAIKI2:
			aosuke.modecount--;
			if(aosuke.modecount==0){
				aosuke.status=TAIKI;
			}
	}
	switch (aosuke.status){
		case NAWABARI:
			targetx=NAWABARIAOSUKEX*8*256;
			targety=NAWABARIAOSUKEY*8*256;
			break;
		case IJIKE:
			targetx=MONSTERHOUSEX*8*256;
			targety=MONSTERHOUSEY*8*256;
			break;
		case OIKAKE: //Set the target coordinates to a point symmetrical to Akabay centered on Pac-Man.
			targetx=pacman.x/2-akabei.x/4;
			targety=pacman.y/2-akabei.y/4;
			if(targetx>=32768) targetx=0;
			else if(targetx>=16384) targetx=16383;
			if(targety>=32768) targety=0;
			else if(targety>=16384) targety=16383;
			targetx<<=2;
			targety<<=2;
			break;
		case MEDAMA:
			targetx=MONSTERHOUSEX*8*256;
			targety=MONSTERHOUSEY*8*256;
			break;
		case TAIKI:
			targetx=MONSTERHOUSEX*8*256;
			targety=(MONSTERHOUSEY-2)*8*256;
	}
	movemonster(&aosuke,targetx,targety);
}

void moveguzuta()
{
	//Guzta move
	unsigned short targetx,targety;
	short dx,dy;

	if(monsterhuntedtimer>0 && guzuta.status!=MEDAMA) return; //When someone is captured, everything except the eyeballs stops
	switch (guzuta.status){
		case NAWABARI:
		case OIKAKE:
			break;
		case IJIKE:
			guzuta.modecount--;
			if(guzuta.modecount==0){
				guzuta.status=OIKAKE;
				guzuta.speed=monsterspeed;
			}
			break;
		case MEDAMA:
			if((guzuta.x/256)==(MONSTERHOUSEX*8) && (guzuta.y/256)==(MONSTERHOUSEY*8)){ //Arrival at Monster House
				guzuta.status=TAIKI2;
				guzuta.modecount=1;
				guzuta.dir=DIR_UP;
				guzuta.speed=monsterspeed;
			}
			break;
		case TAIKI:
			if((guzuta.x/256)==(MONSTERHOUSEX*8) && (guzuta.y/256)==((MONSTERHOUSEY-2)*8)){ //I left the monster house
				guzuta.status=OIKAKE;
				guzuta.dir=DIR_LEFT;
			}
			break;
		case TAIKI2:
			guzuta.modecount--;
			if(guzuta.modecount==0){
				guzuta.status=TAIKI;
			}
	}
	switch (guzuta.status){
		case NAWABARI:
			targetx=NAWABARIGUZUTAX*8*256;
			targety=NAWABARIGUZUTAY*8*256;
			break;
		case IJIKE:
			targetx=MONSTERHOUSEX*8*256;
			targety=MONSTERHOUSEY*8*256;
			break;
		case OIKAKE: //When you are far away from Pac-Man, it will approach you, but when you get close, it will aim at a different direction.
			dx=(short)(guzuta.x/256/8)-(short)(pacman.x/256/8);
			dy=(short)(guzuta.y/256/8)-(short)(pacman.y/256/8);
			if(dx*dx+dy*dy>10){
				targetx=pacman.x;
				targety=pacman.y;
			}
			else{
				targetx=NAWABARIGUZUTAX*8*256;
				targety=NAWABARIGUZUTAY*8*256;
			}
			break;
		case MEDAMA:
			targetx=MONSTERHOUSEX*8*256;
			targety=MONSTERHOUSEY*8*256;
			break;
		case TAIKI:
			targetx=MONSTERHOUSEX*8*256;
			targety=(MONSTERHOUSEY-2)*8*256;
	}
	movemonster(&guzuta,targetx,targety);
}

void sound(void){
	//Sound effect output
	//Called every 1/60th of a second
	// Give priority to the sounds that are processed later
	unsigned short pr;//timer counter value
	unsigned short monsterspeed2;
	pr=0;
	monsterspeed2=2048/monsterspeed;

//monster pitch
	if(akabei.status==MEDAMA || pinky.status==MEDAMA || aosuke.status==MEDAMA || guzuta.status==MEDAMA){
	//Highlight...pr changes continuously from 2200 to 3000, the faster the speed, the faster the change
		if(monstersoundcount==0){
			monstersound=2200;
			monstersounddif=800/monsterspeed2;
		}
		else monstersound+=monstersounddif;
		pr=monstersound;
		monstersoundcount++;
		if(monstersoundcount>monsterspeed2) monstersoundcount=0;
	}
	else if(akabei.status==IJIKE || pinky.status==IJIKE || aosuke.status==IJIKE || guzuta.status==IJIKE){
	//Ijike...pr changes continuously from 7600 to 3800, the faster the speed, the faster the change
		if(monstersoundcount==0){
			monstersounddif=7600/monsterspeed2;
			monstersound=3800+(monsterspeed2>>1)*monstersounddif;
		}
		else monstersound-=monstersounddif;
		pr=monstersound;
		monstersoundcount++;
		if(monstersoundcount*2>monsterspeed2) monstersoundcount=0;
	}
	else{
	//usually
		if(monstersoundcount==0){
			monstersound=2500+monsterspeed2*60;
			monstersounddif=monstersound/monsterspeed2;
		}
		else if(monstersoundcount<monsterspeed2) monstersound+=monstersounddif;
		else monstersound-=monstersounddif;
		pr=monstersound;
		monstersoundcount++;
		if(monstersoundcount>monsterspeed2*2) monstersoundcount=0;
	}

//the sound of eating food
	if(cookiesoundcount!=0){
		if((cookie&1)==0){
			if(cookiesoundcount>3) pr=32000;
			else pr=16000;
		}
		else{
			if(cookiesoundcount>3) pr=16000;
			else pr=32000;
		}
		cookiesoundcount--;
	}
//Sound of eating fruit 0.3 seconds
	if(fruitsoundcount!=0){
		pr=fruitsound;
		fruitsound-=670;
		fruitsoundcount--;
	}
//Sound of eating Ijike 60/60 seconds, 8 times F → upper A, sound once every 2 times and change the pitch
	if(monsterhuntedtimer>0){
		if(monsterhuntedtimer&1){
			pr=monsterhuntedsound;
			monsterhuntedsound-=50;
		}
	}
//10000 Cross the point, Pac-Man +1 Sound G 8 times
	if(over10000soundcount!=0){
		if((over10000soundcount&7)<6) pr=4565;
		over10000soundcount--;
	}

	sound_on(pr/14); //Actual cycle change
}

void erasechars2(_Character *p){
// 1 Rewrite including the area around where the character is displayed
	unsigned short x,y;
	unsigned char x2,y2;

	x=p->x/256;
	y=p->y/256;
	x2=(unsigned char)(x/8);
	y2=(unsigned char)(y/8);
	if((y%8)<3 && y2>0){
		if((x%8)<4 && x2>0) putmapchar(x2-1,y2-1);
		putmapchar(x2,y2-1);
		if(x2<MAPXSIZE-1) putmapchar(x2+1,y2-1);
		if((x%8)>=5 && x2<MAPXSIZE-2) putmapchar(x2+2,y2-1);
	}
	if((x%8)<4 && x2>0) putmapchar(x2-1,y2);
	putmapchar(x2,y2);
	if(x2<MAPXSIZE-1) putmapchar(x2+1,y2);
	if((x%8)>=5 && x2<MAPXSIZE-2) putmapchar(x2+2,y2);
	if(y2<MAPYSIZE-1){
		if((x%8)<4 && x2>0) putmapchar(x2-1,y2+1);
		putmapchar(x2,y2+1);
		if(x2<MAPXSIZE-1) putmapchar(x2+1,y2+1);
		if((x%8)>=5 && x2<MAPXSIZE-2) putmapchar(x2+2,y2+1);
	}
	if((y%8)>=6 && y2<MAPYSIZE-2){
		if((x%8)<4 && x2>0) putmapchar(x2-1,y2+2);
		putmapchar(x2,y2+2);
		if(x2<MAPXSIZE-1) putmapchar(x2+1,y2+2);
		if((x%8)>=5 && x2<MAPXSIZE-2) putmapchar(x2+2,y2+2);
	}
}
void erasechars(){
	//Erase the displayed object (return to the original displayed object)
	erasechars2(&pacman);
	erasechars2(&akabei);
	erasechars2(&pinky);
	erasechars2(&aosuke);
	erasechars2(&guzuta);
}

void putpowercookies(){
	// For redisplaying when changing the power bait color palette
	putmapchar(POWERCOOKIEX1,POWERCOOKIEY1);
	putmapchar(POWERCOOKIEX2,POWERCOOKIEY2);
	putmapchar(POWERCOOKIEX3,POWERCOOKIEY3);
	putmapchar(POWERCOOKIEX4,POWERCOOKIEY4);
}
void displaychars(){
	// Fruits, Ijike, Pac-Man, and monsters other than Ijike are displayed in that order.
	blinkpowercookie(); //power bait flashing
	putpowercookies();
	if(fruitcount>0) putfruit();
	else if(fruitscoretimer>0) putbmpmn3(FRUITX*8-4,FRUITY*8,XWIDTH_SCORE,YWIDTH_SCORE,Scorebmp[4+fruitno]);
	if(akabei.status==IJIKE) putmonster(&akabei);
	if(aosuke.status==IJIKE) putmonster(&aosuke);
	if(guzuta.status==IJIKE) putmonster(&guzuta);
	if(pinky.status==IJIKE) putmonster(&pinky);
	if(monsterhuntedtimer!=0)//Score display when eating Ijike
		putbmpmn3((int)(pacman.x/256)-4,(int)(pacman.y/256),XWIDTH_SCORE,YWIDTH_SCORE,Scorebmp[huntedmonster-1]);
	else putpacman();
	if(akabei.status!=IJIKE) putmonster(&akabei);
	if(aosuke.status!=IJIKE) putmonster(&aosuke);
	if(guzuta.status!=IJIKE) putmonster(&guzuta);
	if(pinky.status!=IJIKE) putmonster(&pinky);
}
void fruitcheck(){
	//Fruit appearance, disappearance check,
	if(fruitflag1 && cookie==FRUITTIME1){
		fruitflag1=0;
		setfruit(1);
		fruitcount=TIMER_FRUIT;
	}
	else if(fruitflag2 && cookie==FRUITTIME2){
		fruitflag2=0;
		setfruit(1);
		fruitcount=TIMER_FRUIT;
	}
	else if(fruitcount>0){
		fruitcount--;
		if(fruitcount==0) setfruit(0);//Delete fruit when time is up
	}
	else if(fruitscoretimer>0){ //Clear score when eating fruit
		fruitscoretimer--;
		if(fruitscoretimer==0){
			putmapchar(FRUITX-1,FRUITY);
			putmapchar(FRUITX  ,FRUITY);
			putmapchar(FRUITX+1,FRUITY);
		}
	}
}

void movechars(){
	//Movement of each character
	movepacman();
	moveakabei();
	movepinky();
	moveaosuke();
	moveguzuta();
	if(monsterhuntedtimer>0) monsterhuntedtimer--;
}

void setmonsterijike(_Character *p){
	//make a monster tame
	if(p->status==MEDAMA) return;
//	if(p->status==TAIKI || p->status==TAIKI2) return;
	p->dir=(p->dir+2)&3; //Reverse direction
	p->turn=0;
	if(ijiketime!=0){
		p->status=IJIKE;
		p->modecount=ijiketime;
		p->speed=ijikespeed;
	}
}

unsigned char monsterhuntcheck(_Character *p){
	//　Monster capture check
	// Return value 0: No capture, 1: Ijike captured, 2: Pac-Man was killed by a monster
#define HANTEI 7 //Capture judgment value that determines how much the coordinates overlap to determine capture

	short dx,dy;

	if(p->status==MEDAMA) return 0;
	dx=(short)(p->x/256)-(short)(pacman.x/256);
	dy=(short)(p->y/256)-(short)(pacman.y/256);
	if(dx!=0 && dy!=0) return 0;
	if(dx<=-(XWIDTH_MONSTER-HANTEI) || dx>=XWIDTH_PACMAN-HANTEI ||
	   dy<=-(YWIDTH_MONSTER-HANTEI) || dy>=YWIDTH_PACMAN-HANTEI) return 0;
	if(p->status==IJIKE){
		p->status=MEDAMA;
		p->x&=0xfeff; //Because the eyeball movement speed skips one dot, force it to even coordinates.
		p->y&=0xfeff; //
		huntedmonster++;
		p->speed=medamaspeed;
		score+=(1<<huntedmonster)*10;
		monsterhuntedtimer=TIMER_HUNTEDSTOP;//stop time
		monsterhuntedsound=5100;
		return 1;
	}
	gamestatus=2;
	return 2;
}
void huntedcheck(void){
	//　Various capture and collision checks

	unsigned char d,x,y;

	if(monsterhuntedtimer>0) return; //All but the eyeballs are stopped
	x=pacman.x/256;
	y=pacman.y/256;
	if((x%8)==4 || (y%8)==4){
		x/=8;
		y/=8;
		if(pacman.dir==DIR_RIGHT) x++;
		else if(pacman.dir==DIR_DOWN) y++;
		d=GETMAP(x,y);
		if(d==MAP_COOKIE){
			//I ate the food
			SETMAP(x,y,MAP_NONE);
			score++;
			cookie--;
			cookiesoundcount=4;
		}
		else if(d==MAP_POWERCOOKIE){
			//I ate power bait
			SETMAP(x,y,MAP_NONE);
			score+=5;
			cookie--;
			cookiesoundcount=4;
			huntedmonster=0;
			setmonsterijike(&akabei);
			setmonsterijike(&pinky);
			setmonsterijike(&aosuke);
			setmonsterijike(&guzuta);
		}
		else if(d==MAP_FRUIT){
			//ate fruit
			getfruit();
		}
		if(cookie==0){
			//Clear the stage
			gamestatus=3;
			return;
		}
	}
	if(akabei.status==IJIKE && monsterhuntcheck(&akabei)) return;
	if(pinky.status==IJIKE && monsterhuntcheck(&pinky)) return;
	if(aosuke.status==IJIKE && monsterhuntcheck(&aosuke)) return;
	if(guzuta.status==IJIKE && monsterhuntcheck(&guzuta)) return;
	if(upflag==0 && score>=1000){
		upflag=1;
		player++;
		displayplayers();
		over10000soundcount=63;
	}
	if(akabei.status!=IJIKE && monsterhuntcheck(&akabei)) return;
	if(pinky.status!=IJIKE && monsterhuntcheck(&pinky)) return;
	if(aosuke.status!=IJIKE && monsterhuntcheck(&aosuke)) return;
	if(guzuta.status!=IJIKE && monsterhuntcheck(&guzuta)) return;
}

// Coffee break 1-3
void coffeebreak1(void){
	unsigned char i;
	unsigned char a1,ac1,av1;
	unsigned char a2,ac2;
	int pacx,akax,pacspeed,akaspeed;
	clearscreen();
	pacx=255*256;
	akax=300*256;
	ac1=5;
	av1=0;
	a2=0;
	ac2=4;
	pacspeed=-290*256/245;
	akaspeed=-320*256/245;
	startmusic(musicdata2);
	while(akax>-20*256){
		ac1--;
		if(ac1==0){
			ac1=5;
			av1++;
			if(av1==6) av1=0;
		}
		if(av1==0) a1=0;
		else if(av1<=3) a1=9+av1;
		else a1=9+6-av1;
		ac2--;
		if(ac2==0){
			ac2=4;
			a2^=1;
		}
		putbmpmn(pacx/256,100,XWIDTH_PACMAN,YWIDTH_PACMAN,Pacmanbmp[a1]);
		putbmpmn(akax/256,101,XWIDTH_MONSTER,YWIDTH_MONSTER,Monsterbmp[6+a2]);
		playmusic60thsec();
		clrbmpmn(pacx/256,100,XWIDTH_PACMAN,YWIDTH_PACMAN);
		clrbmpmn(akax/256,101,XWIDTH_MONSTER,YWIDTH_MONSTER);
		pacx+=pacspeed;
		akax+=akaspeed;
	}

	for(i=0;i<20;i++){
		playmusic60thsec();
	}

	pacx=-75*256;
	akax=-15*256;
	ac1=5;
	av1=0;
	a2=0;
	ac2=4;
	pacspeed=330*256/225;
	akaspeed=300*256/225;
	while(pacx<260*256){
		ac1--;
		if(ac1==0){
			ac1=5;
			av1++;
			if(av1==6) av1=0;
		}
		if(av1<=3) a1=av1;
		else a1=6-av1;
		ac2--;
		if(ac2==0){
			ac2=4;
			a2^=1;
		}
		putbmpmn(pacx/256,83,31,31,Bigpacbmp[a1]);
		putbmpmn(akax/256,101,XWIDTH_MONSTER,YWIDTH_MONSTER,Ijikebmp[a2]);
		playmusic60thsec();
		clrbmpmn(pacx/256,83,31,31);
		clrbmpmn(akax/256,101,XWIDTH_MONSTER,YWIDTH_MONSTER);
		pacx+=pacspeed;
		akax+=akaspeed;
	}
	for(i=0;i<30;i++){
		playmusic60thsec();
	}
}
void coffeebreak2(void){
	unsigned char i;
	unsigned char a1,ac1,av1;
	unsigned char a2,ac2;
	int pacx,akax,pacspeed,akaspeed;
	clearscreen();
	pacx=255*256;
	akax=300*256;
	ac1=5;
	av1=0;
	a2=0;
	ac2=4;
	pacspeed=-290*256/245;
	akaspeed=-320*256/245;
	startmusic(musicdata2+82);
	while(akax>110*256){
		ac1--;
		if(ac1==0){
			ac1=5;
			av1++;
			if(av1==6) av1=0;
		}
		if(av1==0) a1=0;
		else if(av1<=3) a1=9+av1;
		else a1=9+6-av1;
		ac2--;
		if(ac2==0){
			ac2=4;
			a2^=1;
		}
		putbmpmn(122,111,1,4,Pinbmp);
		putbmpmn(pacx/256,100,XWIDTH_PACMAN,YWIDTH_PACMAN,Pacmanbmp[a1]);
		putbmpmn(akax/256,101,XWIDTH_MONSTER,YWIDTH_MONSTER,Monsterbmp[6+a2]);
		playmusic60thsec();
		clrbmpmn(pacx/256,100,XWIDTH_PACMAN,YWIDTH_PACMAN);
		clrbmpmn(akax/256,101,XWIDTH_MONSTER,YWIDTH_MONSTER);
		pacx+=pacspeed;
		akax+=akaspeed;
	}
	akaspeed=-9*256/80;
	while(akax>100*256){
		ac1--;
		if(ac1==0){
			ac1=5;
			av1++;
			if(av1==6) av1=0;
		}
		if(av1==0) a1=0;
		else if(av1<=3) a1=9+av1;
		else a1=9+6-av1;
		ac2--;
		if(ac2==0){
			ac2=4;
			a2^=1;
		}
		putbmpmn(pacx/256,100,XWIDTH_PACMAN,YWIDTH_PACMAN,Pacmanbmp[a1]);
		putbmpmn(akax/256,101,XWIDTH_MONSTER,YWIDTH_MONSTER,Monsterbmp[6+a2]);
		playmusic60thsec();
		clrbmpmn(pacx/256,100,XWIDTH_PACMAN,YWIDTH_PACMAN);
		clrbmpmn(akax/256,101,XWIDTH_MONSTER-1,YWIDTH_MONSTER);
		clrbmpmn(akax/256+XWIDTH_MONSTER-1,101,1,YWIDTH_MONSTER-4);
		pacx+=pacspeed;
		akax+=akaspeed;
	}
	putbmpmn(akax/256,101,22,13,Yabukebmp[0]);
	for(i=0;i<20;i++){
		playmusic60thsec();
	}
	putbmpmn(akax/256,101,22,13,Yabukebmp[1]);
	for(i=0;i<60;i++){
		playmusic60thsec();
	}
}
void coffeebreak3(void){
	unsigned char i;
	unsigned char a1,ac1,av1;
	unsigned char a2,ac2;
	int pacx,akax,pacspeed,akaspeed;
	clearscreen();
	pacx=255*256;
	akax=300*256;
	ac1=5;
	av1=0;
	a2=0;
	ac2=4;
	pacspeed=-290*256/245;
	akaspeed=-320*256/245;
	startmusic(musicdata2);
	while(akax>-20*256){
		ac1--;
		if(ac1==0){
			ac1=5;
			av1++;
			if(av1==6) av1=0;
		}
		if(av1==0) a1=0;
		else if(av1<=3) a1=9+av1;
		else a1=9+6-av1;
		ac2--;
		if(ac2==0){
			ac2=4;
			a2^=1;
		}
		putbmpmn(pacx/256,100,XWIDTH_PACMAN,YWIDTH_PACMAN,Pacmanbmp[a1]);
		putbmpmn(akax/256,101,XWIDTH_MONSTER,YWIDTH_MONSTER,Yabuke2bmp[a2]);
		playmusic60thsec();
		clrbmpmn(pacx/256,100,XWIDTH_PACMAN,YWIDTH_PACMAN);
		clrbmpmn(akax/256,101,XWIDTH_MONSTER,YWIDTH_MONSTER);
		pacx+=pacspeed;
		akax+=akaspeed;
	}

	for(i=0;i<20;i++){
		playmusic60thsec();
	}

	akax=-25*256;
	a2=0;
	ac2=4;
	akaspeed=280*256/225;
	while(akax<260*256){
		ac2--;
		if(ac2==0){
			ac2=4;
			a2^=1;
		}
		putbmpmn(akax/256,101,22,13,Hadakabmp[a2]);
		playmusic60thsec();
		clrbmpmn(akax/256,101,22,13);
		akax+=akaspeed;
	}
	for(i=0;i<30;i++){
		playmusic60thsec();
	}
}
void gamestart(void){
	unsigned int i;
	printstrc(8,15,6,"READY!");
	if(gamestatus==0){ //At the start of the game
		startmusic(musicdata1);//Game start music starts
		for(i=0;i<120;i++){ //First play for 2 seconds
			playmusic60thsec();
		}
		//Display the character and play until the end
		player--;
		displaychars();
		displayplayers();
		while(music.stop==0){
			playmusic60thsec();
		}
	}
	else{
		displaychars();
		displayplayers();
		wait60thsec(120); //2 seconds wait
	}
	printstrc(8,15,0,"      "); //READY erase
}
void deadanim(void){
	//Animation and sound when Pac-Man is defeated
	unsigned char i,j;
	wait60thsec(120); //2 seconds wait
	erasechars();
	if(fruitscoretimer>0){ //Clear score when eating fruit
		putmapchar(FRUITX-1,FRUITY);
		putmapchar(FRUITX  ,FRUITY);
		putmapchar(FRUITX+1,FRUITY);
	}

	for(i=0;i<9;i++){
		erasechars2(&pacman);
		putbmpmn3(pacman.x/256-3,pacman.y/256-3,XWIDTH_PACMAN,YWIDTH_PACMAN,Pacmandeadbmp[i]);
		if(i>1 && i<8){
			for(j=0;j<13;j++){
				sound_on((2400+(i-2)*320+(2400+(i-2)*320)*(12-j)/12)/14);
				wait60thsec(1); // 1/60 seconds wait
	 		}
	 	}
	 	else if(i==8){
			for(j=0;j<13;j++){
				sound_on((1700+1700*(12-j)/12)/14);
	 			wait60thsec(1); // 1/60 seconds wait
	 		}
	 	}
	}
	erasechars2(&pacman);
	for(j=0;j<13;j++){
		sound_on((1700+1700*(12-j)/12)/14);
		wait60thsec(1); // 1/60 seconds wait
	}
	sound_off();//sound stop
}
void putwall(void){
	//Redisplay when wall flashes
	unsigned char *p;
	int i,j;
	p=scenedata;
	for(i=0;i<MAPYSIZE;i++){
		for(j=0;j<MAPXSIZE;j++){
			if(*p<=0x8d){
				printchar(j,i,COLOR_WALL,*p);
			}
			p++;
		}
	}
}
void stageclear(void){
	//Animation when clearing the surface
	int i;
	pacman.animvalue=0; //Display something with the mouth forcibly closed
	pacman.animcount=pacman.animcount0;
	putpacman();
 	wait60thsec(120); //2 seconds wait
	erasechars();
	if(fruitscoretimer>0){ //Clear score when eating fruit
		putmapchar(FRUITX-1,FRUITY);
		putmapchar(FRUITX  ,FRUITY);
		putmapchar(FRUITX+1,FRUITY);
	}
	putpacman();
	for(i=1;i<=6;i++){
 		wait60thsec(10); // 10/60 seconds wait
		set_palette(COLOR_WALL,255,255,255); //white wall color
		putwall();
		putpacman();
 		wait60thsec(10); // 10/60 seconds wait
		set_palette(COLOR_WALL,250,20,40); //restore wall color
		putwall();
		putpacman();
	}
	switch(stage){ //coffee break check
		case 2:
			coffeebreak1();
			break;
		case 5:
			coffeebreak2();
			break;
		case 9:
		case 13:
		case 17:
			coffeebreak3();
	}
}

void gameover(){
	//When the game is over
	printstrc(6,10,2,"GAME OVER");
	wait60thsec(180); // 3 seconds wait
}

void title(void){
	//タイトル画面表示
	int i;
	unsigned char a1,ac1,av1;
	unsigned char a2,ac2;
	int pacx,akax,pacspeed,akaspeed;

	while(1){
		clearscreen();

		printstrc(5,2,2,"1UP");
		printstrc(18,2,2,"HI-SCORE");
		printscore(4,3,7,score);
		printchar(10,3,7,'0');
		printscore(19,3,7,highscore);
		printchar(25,3,7,'0');

		//Logo display
		putbmpmn(63,80,114,36,Titlelogobmp[0]);
		printstrc( 4,20,7,"STM32 TFT ISMAIL BASIC");
		printstrc(10,22,7,"BY ISMAIL");
		printstrc( 6,25,4,"PUSH START BUTTON");

		if(startkeycheck(600)) return;//10 seconds wait

		clearscreen();
		printstrc(5,2,2,"1UP");
		printstrc(18,2,2,"HI-SCORE");
		printscore(4,3,7,score);
		printchar(10,3,7,'0');
		printscore(19,3,7,highscore);
		printchar(25,3,7,'0');
		printstrc(4,6,7,"CHARACTER  /  NICKNAME");
		if(startkeycheck(50)) return;
		putbmpmn(3*8-3,8*8-3,XWIDTH_MONSTER,YWIDTH_MONSTER,Monsterbmp[AKABEI*8+2]);
		printstrc(5,8,COLOR_AKABEI,"OIKAKE\x90\x90\x90\x90\x90\x90\x90");
		if(startkeycheck(50)) return;
		printstrc(18,8,COLOR_AKABEI,"\"AKABEI\"");
		if(startkeycheck(50)) return;
		putbmpmn(3*8-3,10*8-3,XWIDTH_MONSTER,YWIDTH_MONSTER,Monsterbmp[PINKY*8+2]);
		printstrc(5,10,COLOR_PINKY,"MACHIBUSE\x90\x90\x90\x90");
		if(startkeycheck(50)) return;
		printstrc(18,10,COLOR_PINKY,"\"PINKY\"");
		if(startkeycheck(50)) return;
		putbmpmn(3*8-3,12*8-3,XWIDTH_MONSTER,YWIDTH_MONSTER,Monsterbmp[AOSUKE*8+2]);
		printstrc(5,12,COLOR_AOSUKE,"KIMAGURE\x90\x90\x90\x90\x90");
		if(startkeycheck(50)) return;
		printstrc(18,12,COLOR_AOSUKE,"\"AOSUKE\"");
		if(startkeycheck(50)) return;
		putbmpmn(3*8-3,14*8-3,XWIDTH_MONSTER,YWIDTH_MONSTER,Monsterbmp[GUZUTA*8+2]);
		printstrc(5,14,COLOR_GUZUTA,"OTOBOKE\x90\x90\x90\x90\x90\x90");
		if(startkeycheck(50)) return;
		printstrc(18,14,COLOR_GUZUTA,"\"GUZUTA\"");
		if(startkeycheck(50)) return;
		printchar(11,21,COLOR_COOKIE,CODE_COOKIE);
		printstrc(13,21,7,"10 PTS");
		printchar(11,22,COLOR_COOKIE,CODE_POWERCOOKIE);
		printstrc(13,22,7,"50 PTS");
		printstrc(6,25,4,"PUSH START BUTTON");

		gamecount=0;
		pacx=239*256;
		akax=264*256;
		ac1=5;
		av1=0;
		a2=0;
		ac2=4;
		pacspeed=-200;
		akaspeed=-200;
		while(pacx>46*256){
			ac1--;
			if(ac1==0){
				ac1=5;
				av1++;
				if(av1==6) av1=0;
			}
			if(av1==0) a1=0;
			else if(av1<=3) a1=9+av1;
			else a1=9+6-av1;
			ac2--;
			if(ac2==0){
				ac2=4;
				a2^=1;
			}
			printchar(6,17,COLOR_POWERCOOKIE,CODE_POWERCOOKIE);
			putbmpmn(pacx/256,133,XWIDTH_PACMAN,YWIDTH_PACMAN,Pacmanbmp[a1]);
			putbmpmn(akax/256,134,XWIDTH_MONSTER,YWIDTH_MONSTER,Monsterbmp[AKABEI*8+6+a2]);
			putbmpmn(akax/256+18,134,XWIDTH_MONSTER,YWIDTH_MONSTER,Monsterbmp[PINKY*8+6+a2]);
			putbmpmn(akax/256+36,134,XWIDTH_MONSTER,YWIDTH_MONSTER,Monsterbmp[AOSUKE*8+6+a2]);
			putbmpmn(akax/256+54,134,XWIDTH_MONSTER,YWIDTH_MONSTER,Monsterbmp[GUZUTA*8+6+a2]);
			if(startkeycheck(1)) return;
			clrbmpmn(pacx/256,133,XWIDTH_PACMAN,YWIDTH_PACMAN);
			clrbmpmn(akax/256,134,XWIDTH_MONSTER+54,YWIDTH_MONSTER);
			pacx+=pacspeed;
			akax+=akaspeed;
			gamecount++;
			blinkpowercookie();
		}
		pacspeed=-pacspeed;
		akaspeed=100;
		i=0;
		while(i<4){
			ac1--;
			if(ac1==0){
				ac1=5;
				av1++;
				if(av1==6) av1=0;
			}
			if(av1==0) a1=0;
			else if(av1<=3) a1=3+av1;
			else a1=3+6-av1;
			ac2--;
			if(ac2==0){
				ac2=4;
				a2^=1;
			}
			if(i<=0) putbmpmn(akax/256,134,XWIDTH_MONSTER,YWIDTH_MONSTER,Ijikebmp[a2]);
			if(i<=1) putbmpmn(akax/256+18,134,XWIDTH_MONSTER,YWIDTH_MONSTER,Ijikebmp[a2]);
			if(i<=2) putbmpmn(akax/256+36,134,XWIDTH_MONSTER,YWIDTH_MONSTER,Ijikebmp[a2]);
			if(i<=3) putbmpmn(akax/256+54,134,XWIDTH_MONSTER,YWIDTH_MONSTER,Ijikebmp[a2]);
			putbmpmn(pacx/256,133,XWIDTH_PACMAN,YWIDTH_PACMAN,Pacmanbmp[a1]);
			if(startkeycheck(1)) return;
			clrbmpmn(pacx/256,133,XWIDTH_PACMAN,YWIDTH_PACMAN);
			switch(i){
				case 0:
					if(pacx/256>akax/256-6){
						i++;
						clrbmpmn(akax/256,134,XWIDTH_MONSTER,YWIDTH_MONSTER);
						putbmpmn(pacx/256,136,XWIDTH_SCORE,YWIDTH_SCORE,Scorebmp[0]);
						if(startkeycheck(30)) return;
					}
					break;
				case 1:
					if(pacx/256>akax/256+18-6){
						i++;
						clrbmpmn(akax/256+18,134,XWIDTH_MONSTER,YWIDTH_MONSTER);
						putbmpmn(pacx/256,136,XWIDTH_SCORE,YWIDTH_SCORE,Scorebmp[1]);
						if(startkeycheck(30)) return;
					}
					break;
				case 2:
					if(pacx/256>akax/256+36-6){
						i++;
						clrbmpmn(akax/256+36,134,XWIDTH_MONSTER,YWIDTH_MONSTER);
						putbmpmn(pacx/256,136,XWIDTH_SCORE,YWIDTH_SCORE,Scorebmp[2]);
						if(startkeycheck(30)) return;
					}
					break;
				case 3:
					if(pacx/256>akax/256+54-6){
						i++;
						clrbmpmn(akax/256+54,134,XWIDTH_MONSTER,YWIDTH_MONSTER);
						putbmpmn(pacx/256,136,XWIDTH_SCORE,YWIDTH_SCORE,Scorebmp[3]);
						if(startkeycheck(30)) return;
					}
			}
			clrbmpmn(akax/256,134,XWIDTH_MONSTER+54,YWIDTH_MONSTER);
			pacx+=pacspeed;
			akax+=akaspeed;
		}
	}
}

void game(void){
	gameinit2();//Initialize scores etc.
	gamestatus=0;//0: Game start, 1: Game in progress, 2: Player decreased by 1, 3: Stage cleared, 4: Game over
	while(gamestatus<4){
		gameinit3();//Scene clear, initial passageway display
		if(gamestatus==3) gamestatus=1;
		while(gamestatus<3){
			gamecount=0;//whole counter
			gameinit4();//Pacman, monster position initialization, etc.
			gamestart();//Ready! display
			gamestatus=1;
			while(gamestatus==1){
				wait60thsec(1);
				gamecount++;
				sound();		//sound effect output
				erasechars();	//Character display erase
				keycheck();		//Button press check
				movechars();	//character movement
				displaychars();	//Character display
				fruitcheck();	//Fruit related check
				huntedcheck();	//Ate, eaten check
				displayscore();	//Score display
			}
			sound_off();//sound stop
			set_palette(COLOR_POWERCOOKIE,0,255,255);//Return to power bait color standard
			if(gamestatus==2){
				deadanim(); //Animation when Pacman is defeated
				if(player==0) gamestatus=4;//game over
				else player--;
			}
			else if(gamestatus==3){
				//Clear the stage
				stageclear(); //Animation when clearing a stage, coffee break
			}
		}
	}
	gameover(); //Game end processing
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
	init_graphic(); //Start of using LCD
	LCD_WriteComm(0x37); //Scroll settings to center the screen
	LCD_WriteData2(272);

	// Initialize joystick - read initial position
	ReadJoystick();

	highscore=1000;
	gameinit(); //Initialize the entire game
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
		title();//Title screen, return with start button
		game();//game main loop
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;  // PA0 - X axis
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_DC_Pin|LCD_RES_Pin|LCD_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : LCD_DC_Pin LCD_RES_Pin */
  GPIO_InitStruct.Pin = LCD_DC_Pin|LCD_RES_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_CS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LCD_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : START_Pin (keep for compatibility) and Joystick button */
  GPIO_InitStruct.Pin = START_Pin|JOYSTICK_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  // Note: PA0 and PA1 are configured for ADC in MX_ADC1_Init()

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
