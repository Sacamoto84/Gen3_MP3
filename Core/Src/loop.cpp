#include "main.h"
#include "stdio.h"
#include "logUART.h"
#include "global.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "fatfs.h"

#include "mp3dec.h"

#include "mString.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

extern void init_DAC_DMA(void);
extern void DAC_DMA_Play(void);
extern void PAGE_init_palitra(void); //Инициализация палитры

extern int U3_DMA_TX_Complete;
extern HMP3Decoder  hMP3Decoder;		// указатель на ОЗУ декодера
extern FIL          mp3_file;		    	    // указатель на играемый файл

void stopError(void);

classLog rtt;
TFT tft;
GFXFONT gfxfont;

void loop(){while (1){


	task_HMI();

	rtt.info ("Свободно памяти %d", xPortGetFreeHeapSize());
	//if (myTaskMP4Handle)

	osDelay(20000);
}}


HAL_SD_CardInfoTypeDef Card_Info;

volatile FRESULT fres;





void setup(void) {

	PAGE_init_palitra();

	rtt.init(&huart3);
	rtt.dma_completed  = &U3_DMA_TX_Complete;
	*rtt.dma_completed = 1;
	rtt.useDMA = false;

	rtt.clear();

	rtt.setBold();
    rtt.info("MP3 V1.0.2");
    rtt.reset();

	TimerDWT.init();
	TimerT5.init(&htim5);

	//HAL_Delay(1000);

	__HAL_SPI_DISABLE(&hspi1);
	SPI1->CR1 |= (0x1UL << (5U));
	__HAL_SPI_ENABLE(&hspi1);

	tft.init(&LCD_0);

    __HAL_SPI_DISABLE(&hspi1);
    SPI1->CR1 &= ~(0x1UL << (5U));
    __HAL_SPI_ENABLE(&hspi1);

    //tft.LCD->MADCTL.RGB = 1;
    //tft.ST77XX_Update_MADCTL();

	tft.setResStartAdress(0x08020000); //Установим начало ресурсов

	gfxfont.init(&tft);

	gfxfont.setFont(&CourierCyr12pt8b);
	//gfxfont.setFont(&JetBrainsMono_VariableFont_wght12pt8b);
	tft.SetFontColor(4, 14);


    //tft.setColorToPalete(0,tft.RGB565(255,4,6));
    //tft.setColorToPalete(1,tft.RGB565(1,56,33));

	tft.Fill(0);
	tft.RectangleFilled(10, 10, 20, 20, 1);
	tft.Update();

	rtt.info("Start testing SDCARD");
	rtt.color(87);
	BSP_SD_GetCardInfo(&Card_Info);
	//rtt.print("Card Type               -> %s\n", &card_type[Card_Info.CardType][0]);
	//rtt.print("Card Version            -> %s\n", &card_ver[Card_Info.CardVersion][0]);
	rtt.print("Block Size      -> 0x%x\n", (int)Card_Info.BlockSize);
	rtt.print("Capacity blocks -> 0x%x(%uGB)\n", (int)Card_Info.BlockNbr,(int)((((float)Card_Info.BlockNbr/1000)*(float)Card_Info.BlockSize/1000000)+0.5));
    rtt.reset();
	rtt.warning("Монтирования диска") ; // ("монтирования диска\n");

	// смонтировать диск
	FRESULT result = f_mount(&SDFatFS, SDPath, 1); //Mount MicroSd
	if (result != FR_OK)
	{
      rtt.error("Ошибка монтирования диска %d", result);
	  stopError();
	}
	else
	  rtt.successful("Монтирование диска: успешно");

	//Открыть файл 'GIFtoBIN.exe'
	result = f_open(&SDFile, "GIFtoBIN.exe", FA_READ);
	if (result == FR_OK)
	{
	  rtt.successful("GIFtoBIN.exe..OK");
	  f_close(&SDFile);
	}
	else
	  rtt.error("GIFtoBIN.exe..Ошибка открытия файла");

	globalPach = "/music";
	readDir((char *) globalPach.c_str(), &list_mp3);
	list_mp3.root =  false;
	addTreeDot(&list_mp3);

	rtt.info ("Свободно памяти Всего %d", xPortGetFreeHeapSize());

	// выделить память для декодера
	//hMP3Decoder = MP3InitDecoder(); //23888 байт
	//if (hMP3Decoder == NULL)
	//{
	//	rtt.error("Ошибка выделения памяти для mp3-декодера");
	//	stopError();
	//}
	//rtt.successful("mp3DecoderState->hMP3Decoder = %x", (unsigned int)hMP3Decoder);




	//myTaskMP4Handle = osThreadNew(StartTaskMP3, (char *)"xxx.mp3", &myTaskMP3_attributes);

	//rtt.info ("StartTaskMP4 Свободно памяти %d", xPortGetFreeHeapSize());

	//play((char*)"1.mp3");

	//static decoder_param_t decoder_param;
	//decoder_param.file = &file;
	//decoder_param.DAC_interface = iDAC;
	//decoder_param.playCmdMailBox = cmdEventMailbox;
    //mp3_player_init(decoder_param_t * decoder_param);

}

extern "C" void main_cpp(void) {
	setup();
	loop();
}





