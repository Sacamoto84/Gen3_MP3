#include "global.h"

PlayerInfo playerInfo;

mString <64> globalPach;

Dir_File_Info_Array list_mp3 CCMRAM;

HiSpeedDWT TimerDWT CCMRAM;
HiSpeedDWT TimerT5  CCMRAM;

Encoder_typedef Encoder;

GButton KEY(ENTER_GPIO_Port, ENTER_Pin);

uint16_t palitra[16];

extern "C" void KEY_tick(void)
{
  KEY.tick();
}


void PAGE_init_palitra(void) {
	palitra[0] = pColor_0;
	palitra[1] = pColor_1;
	palitra[2] = pColor_2;
	palitra[3] = pColor_3;
	palitra[4] = pColor_4;
	palitra[5] = pColor_5;
	palitra[6] = pColor_6;
	palitra[7] = pColor_7;
	palitra[8] = pColor_8;
	palitra[9] = pColor_9;
	palitra[10] = pColor_10;
	palitra[11] = pColor_11;
	palitra[12] = pColor_12;
	palitra[13] = pColor_13;
	palitra[14] = pColor14_WHITE;
	palitra[15] = pColor15_BLACK;
}

//ST7789 135 240 16 bit
uint8_t LCD_Buffer8[240 * 240/ 2] CCMRAM;
//uint16_t LCD_Buffer16_0[1];
uTFT_LCD_t LCD_0 = {
		240,                  // Ширина экрана
		240,                  // Высота экрана
		ST7789,               // Драйвер
		4,                    // bit
		&hspi1,               // Spi
		NULL,	              // I2C
		NULL,                 // 16 бит буффер
		&LCD_Buffer8[0],      // !16 бит буффер
		&palitra[0],          // Палитра
		0,                    // Смещение по X
		0,                    // Смещение по Y
		0,                    // CS PORT
		0,                    // CS PIN
		A0_GPIO_Port,         // DC PORT
		A0_Pin,               // DC PIN
		RESET_GPIO_Port,      // RESET PORT
		RESET_Pin,	          // RESET PIN
		0, {                     //MADCTL
		0,   //MY
				0,   //MX
				0,   //MV
				0,   //ML
				0,   //RGB
				0,   //MH
		}, {                     //Смещение но X Y
		240, 240, 0,   //DX_0
				0,   //DY_0
				240, 240, 0,   //DX_90
				0,   //DY_90
				240, 240, 0,	 //DX_180
				0,   //DY_180
				240, 240, 0,   //DX_270
				0,   //DY_270
		} };


