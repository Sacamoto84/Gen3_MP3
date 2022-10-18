/*
 * global.h
 *
 *  Created on: 10 окт. 2022 г.
 *      Author: Ivan
 */

#ifndef INC_GLOBAL_H_
#define INC_GLOBAL_H_

	#include "stm32f4xx_hal.h"	    //│
	#include "stm32f4xx.h"          //│
	#include "stm32f4xx_it.h"  	    //│
	#include "main.h"          		//│
	#include "crc.h"                //│
	#include "dma.h"      	        //│
	#include "i2c.h"      	        //│
	#include "spi.h"                //│
	#include "tim.h"       	  	    //│
	#include "gpio.h"               //│
	#include "TFT.h"            	//│
	#include "gfxfont.h"            //│
	#include "gfx_fonts_includes.h" //│
	#include "HiSpeedDWT.h"         //│
    #include "logUART.h"
    #include "GyverButton.h"
    #include "mString.h"


#define pColor_0       tft.RGB565(158,158,158)    // Фон заливки, окантовки
#define pColor_1       tft.RGB565(77,85,74)       // Фон основной задний фон
#define pColor_2       tft.RGB565(148,165,142)    // Светлый фон выбранной кнопки
#define pColor_3       tft.RGB565(208,231,199)    // Светлая часть окантовки светлой кнопки
#define pColor_4       tft.RGB565(89,99,85)       // Темная часть окантовки светлой кнопки
#define pColor_5       tft.RGB565(166,170,165)    // Светлая часть окантовки темной кнопки
#define pColor_6       tft.RGB565(46,51,44)       // Темная часть окантовки темной кнопки
#define pColor_7       tft.RGB565(16,16,16)
#define pColor_8       0
#define pColor_9       tft.RGB565(209,209,209)
#define pColor_10      tft.RGB565(0,139,139)

#define pColor_11      tft.RGB565(255,0,0)
#define pColor_12      tft.RGB565(0,255,0)
#define pColor_13      tft.RGB565(0,0,255)

#define pColor14_WHITE tft.RGB565(255,255,255)    // WHITE
#define pColor15_BLACK tft.RGB565(0,0,0)          // BLACK


extern mString <64> globalPach;

typedef struct{


	mString <24> filename;


	uint32_t uwTick;
	ulong timeCurrent;     //Текущее время воспроизведения
    char timeCurrentStr [16]; //Текст для вывода текста времени

    float fpersent;           //Процент прочтения файла 0..100 по положению поинтера в файле
    int   flseekcurrent;      //Текущее положение в файле
    int   filesize;           //размер текущего файла

    float gain;//

    int error;  //Количество ощибок

    //Расчет текущего положения в файле
    void calculatePersent(void)  { fpersent = (float)flseekcurrent/(float)filesize; }

} PlayerInfo;


typedef struct
{
  uint32_t  maxFileCount = 0;   //Количество найденных файлов
  mString   <24> name [32];     //Имя файла
  uint      size [32];          //Размер файла
  bool      isDirectory[32];    //Признак того что это директория
  bool      subDirectory;       //Признак того мы в подпапке
  bool      root;               //Признак того что это root и не нужно делать возврат
  bool      rezerv;             //Резерв

} Dir_File_Info_Array;

extern Dir_File_Info_Array list_mp3 CCMRAM;

typedef struct
{
  uint8_t Left;
  uint8_t Right;
}Encoder_typedef;

extern Encoder_typedef Encoder;
extern GButton KEY;

extern PlayerInfo playerInfo;

extern uint16_t palitra[];
extern TFT tft ;
extern classLog rtt;
extern GFXFONT gfxfont;

extern HiSpeedDWT TimerDWT;
extern HiSpeedDWT TimerT5;

extern uTFT_LCD_t LCD_0;


extern void PAGE_init_palitra(void);
extern void task_HMI(void);

// utiletes.cpp
extern void sort(mString <24> *S, int N);
extern void readDir(char * path, Dir_File_Info_Array * list);
extern void stopError(void);
extern mString <64> dirForvard(mString <64> dir);
extern void addTreeDot(Dir_File_Info_Array * list);

extern void play(char * name);

#endif /* INC_GLOBAL_H_ */
