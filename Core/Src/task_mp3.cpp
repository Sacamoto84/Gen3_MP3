#include "global.h"
#include "mp3dec.h"
#include "fatfs.h"
#include "playerTask.h"
#include "debugTask.h"
#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

#define MP3_GAIN 1.8F



static bool taskMP3_terminate;


extern uint DMA_Buffer_Current;

static void stop_decode(cmd_t player_cmd);

#define E_OK                0

#define MP3_TASK_STK_SIZE	512			// размер стека задачи 2кб
#define MP3_FILEBUFF_SIZE	4096		// размер файлового буфера
#define DAC_BUFFER_SIZE		(1152*2)	// размер выходного буфера (16-битных слов) для одного фрейма *2 канала

#define MONO_SUPPORT

extern void stopError(void);
extern void DAC_setSampleRate(int sample = 44000);
extern void DAC_DMA_Pause(void);
extern void init_MP3_DAC_DMA(void);
extern void DAC_DMA_Play(void);
extern void DAC_DMA_ClearBuffer(void);

const osThreadAttr_t myTaskMP3_attributes = {
	  .name = "TaskMP3",
	  .stack_size = 512 * 4,
	  .priority = (osPriority_t) osPriorityNormal,
	};

osThreadId_t myTaskMP3Handle;

static void MP3_Deinit(void);

decodeStatistic_t decodeStatistic;
debug_mode_t      debug_mode;		    // отладочные флаги

extern int16_t   outBuff[2][1152*2] RAM_16; // буфер выходного потока

/// результат чтения потока MP3 из файла для функции
typedef enum { READ_OK =  0, READ_ERROR	= -1, READ_END_DATA	= -2} read_result_t;

typedef struct
{

	// файловые переменные
	//FIL     *mp3_file;					    // указатель на играемый файл

	uint32_t fileAddr;						// адрес первого непрочитанного из файла байта

	// переменные для декодера
	HMP3Decoder  hMP3Decoder;				// указатель на ОЗУ декодера
	MP3FrameInfo mp3FrameInfo;				// параметры фрейма

	// буферы
	uint32_t  t[3];
	uint8_t   inputBuf[MP3_FILEBUFF_SIZE];	// буфер для входного потока

	uint32_t  outBuffPtr;					// указатель на свободный буфер

	uint8_t   * inputDataPtr;	// указатель начала данных в файловом буфере
	int32_t   bytes_left;		// указатель количества данных в файловом буфере
	 int32_t  samprate;
	uint32_t  frameCNT;			// счётчик декодированных фреймов

	// локальные переменные для управления ЦАП и задачей проигрывателя
	//cmd_t	 	player_cmd;
	//OS_FlagID 	dac_DataReqFlag;
	//OS_FlagID 	player_CmdMailBox;
	//dac_cmd_i 	* DAC_interface;

	uint32_t	summaryDecodeTime_mks;
	uint32_t	DecodeTime_Ticks;

	float gain; //Усиление

} mp3DecoderState_t;

//*-----------------------------------------------------------------------------------------------
//*			Переменные
//*-----------------------------------------------------------------------------------------------

//malloc
static mp3DecoderState_t * mp3DecoderState;	// переменная состояния, для которой выделяется место в куче

// выделить память для декодера
//mp3DecoderState->hMP3Decoder = MP3InitDecoder();

void MP3Task(void);

/*************************************************************************************************
 * @brief	Инициализация задачи-декодера MP3
 *
 * @param 	decoder_params - указатель на интерфейс управления проигрывателем
 * @return	указатель на функцию деинициализации декодера
 ************************************************************************************************/
void mp3_player_init(void)
{
	rtt.info ("Свободно памяти %d", xPortGetFreeHeapSize());

	// выделить память для задачи
	mp3DecoderState = (mp3DecoderState_t*)malloc(sizeof(mp3DecoderState_t));

	if (mp3DecoderState == NULL)
	{
		rtt.print("Ошибка выделения памяти для mp3DecoderState\n");
		stopError();
	}
	rtt.successful("OK выделения памяти для mp3DecoderState\n");
	rtt.info ("Свободно памяти %d", xPortGetFreeHeapSize());

	// выделить память для декодера
	mp3DecoderState->hMP3Decoder = MP3InitDecoder();
	if (mp3DecoderState->hMP3Decoder == NULL)
	{
		rtt.print("Ошибка выделения памяти для MP3InitDecoder\n");
		stopError();
	}
	rtt.successful("OK выделения памяти для MP3InitDecoder");
	rtt.info ("Свободно памяти %d", xPortGetFreeHeapSize());

	//mp3DecoderState->mp3_file = &SDFile;

	mp3DecoderState->inputDataPtr = NULL;
	mp3DecoderState->bytes_left = 0;
	mp3DecoderState->frameCNT = 0;
	mp3DecoderState->outBuffPtr = 0;
	mp3DecoderState->samprate = 0;
	mp3DecoderState->fileAddr = 0;

	mp3DecoderState->DecodeTime_Ticks = HAL_GetTick();

}


static read_result_t ReadMP3buff(mp3DecoderState_t * mp3DecoderState)
{
	// если буфер полный, ничего не делать
	if (mp3DecoderState->bytes_left >= MP3_FILEBUFF_SIZE)
		return READ_OK;

	// если в файловом буфере остались данные, переместить их в начало буфера
	if (mp3DecoderState->bytes_left > 0)
	{
		memcpy(mp3DecoderState->inputBuf, mp3DecoderState->inputDataPtr, mp3DecoderState->bytes_left);
	}
	assert_param(mp3DecoderState->bytes_left >= 0 && mp3DecoderState->bytes_left <= MP3_FILEBUFF_SIZE);

	// прочитать очередную порцию данных
	uint32_t bytes_to_read = MP3_FILEBUFF_SIZE - mp3DecoderState->bytes_left;
	uint32_t bytes_read;

	FRESULT result = f_read(&SDFile,
							(BYTE *)mp3DecoderState->inputBuf + mp3DecoderState->bytes_left,
							bytes_to_read,
							(UINT*)&bytes_read);

	mp3DecoderState->fileAddr += bytes_to_read;

	if (result != FR_OK)
	{
		return READ_ERROR;
	}

	mp3DecoderState->inputDataPtr = mp3DecoderState->inputBuf;
	mp3DecoderState->bytes_left += bytes_read;

	if (bytes_read == bytes_to_read)
		return READ_OK;
	else
		return READ_END_DATA;
}

/*************************************************************************************************
 * @brief	Задача декодера MP3
 * @param	pdata - указатель на данные (не используется)
 ***********************************************************************************************/
void MP3(char * mp3name)
{
	rtt.info("MP3:mp3name %s", mp3name);
	debug_mode.showDecoderInfo = true;
	//debug_mode.showFrameDecodeTime = true;

	mp3_player_init();

	init_MP3_DAC_DMA();
    DAC_DMA_ClearBuffer(); //Очистить выходной буффер

    bool init        = true; //Init режим init
    int init_count   = 0;


    //DAC_DMA_Play();


    mString <64> fullPath;
    fullPath = globalPach;
    fullPath += "/";
    fullPath += mp3name;

	//Открыть файл 'GIFtoBIN.exe'
	FRESULT result = f_open(&SDFile, fullPath.c_str() , FA_READ);
	if (result != FR_OK)
	{
	  rtt.error("MP3:%s..Ошибка открытия файла", fullPath.c_str());

	  f_close(&SDFile);
	}
	else
		rtt.successful("MP3:%s..OK", fullPath.c_str());

	memset(&playerInfo, 0, sizeof(PlayerInfo));
	int filesize = f_size(&SDFile);
	rtt.info("MP3:Размер файла %d", filesize);
	playerInfo.filesize = filesize;

	//Текущее положение
	playerInfo.flseekcurrent = f_tell (&SDFile);
	playerInfo.calculatePersent(); //Расчет процента

	uint32_t t = uwTick;

	mp3DecoderState->gain = MP3_GAIN;


	playerInfo.filename = mp3name;

	DMA_Buffer_Current = 0;

	for(;;)
	{


		if(taskMP3_terminate)
		{
			rtt.warning("Завершение MP3 по внешнему запросу");
			taskMP3_terminate = false;
			f_close(&SDFile);
			DAC_DMA_Pause();
			MP3_Deinit();
			osThreadExit();
		}



		if (init == false )
		{
		  //Ждем запроса от DMA
		  while(DMA_Buffer_Current == 0){osDelay(2);}
		  DMA_Buffer_Current = 0;
		}
		else
		  init_count++;


		playerInfo.timeCurrent += uwTick - t;
		t = uwTick;



		//
		  //останова
		//
		StartTimeMeasurement();	// начало контроля времени выполнения

		// прочитать порцию данных
		read_result_t fileReadResult = ReadMP3buff(mp3DecoderState);

		//Текущее положение
		playerInfo.flseekcurrent = f_tell (&SDFile);
		playerInfo.calculatePersent(); //Расчет процента
        playerInfo.gain = mp3DecoderState->gain;

		//rtt.print("Полное время проигрывания файла %u мс\n",(unsigned int)mp3DecoderState->DecodeTime_Ticks);

		////
		if (fileReadResult == READ_ERROR)
		{
			StopTimeMeasurement();
			mp3DecoderState->DecodeTime_Ticks = HAL_GetTick() - mp3DecoderState->DecodeTime_Ticks;

			if (debug_mode.showDecoderInfo)
			{
				rtt.print("Чистое время декодирования файла %u мс\n",(unsigned int)mp3DecoderState->DecodeTime_Ticks);
				uint32_t temp = mp3DecoderState->summaryDecodeTime_mks/1000;
				rtt.print("Длительность проигрывания файла %u мс\n",(unsigned int)temp);
				temp = mp3DecoderState->summaryDecodeTime_mks/10;
				temp *= mp3DecoderState->DecodeTime_Ticks;
				rtt.print("Средняя загрузка контроллера %u%%\n",(unsigned int)temp);
				// отображается время выполнения только одной процедуры - MP3Decode

				rtt.print("Декодировано %u фреймов\n", (unsigned int)mp3DecoderState->frameCNT);
				rtt.print("Ошибка чтения файла\n");
				rtt.print("Error in %s line %u\n", (uint8_t *)__FILE__, (unsigned int)__LINE__);
				rtt.error("Воспроизведение файла завершено по ошибке\n");

				f_close(&SDFile);
				DAC_DMA_Pause();
				MP3_Deinit();
				osThreadExit();
			}
			stop_decode(DECODE_ERROR);
			continue;
		}
		////

		if (fileReadResult == READ_END_DATA)
		{
			mp3DecoderState->DecodeTime_Ticks = HAL_GetTick() - mp3DecoderState->DecodeTime_Ticks;
			StopTimeMeasurement();
			if (debug_mode.showDecoderInfo)
			{
				uint32_t temp = mp3DecoderState->summaryDecodeTime_mks/1000;
				rtt.print("Чистое время декодирования файла %u мс\n",(unsigned int)temp);
				rtt.print("Полное время проигрывания файла %u мс\n",(unsigned int)mp3DecoderState->DecodeTime_Ticks);
				temp = mp3DecoderState->summaryDecodeTime_mks/10;
				temp /= mp3DecoderState->DecodeTime_Ticks;

				rtt.print("Средняя загрузка контроллера %u%%",(unsigned int)temp);
				// отображается время выполнения только одной процедуры - MP3Decode

				rtt.print("Декодировано %u фреймов\n", (unsigned int)mp3DecoderState->frameCNT);
				rtt.info("Воспроизведение файла завершено полностью\n");
			}
			stop_decode(SONG_COMPLETE);
			f_close(&SDFile);
			DAC_DMA_Pause();

			//osThreadId id = osThreadGetId ();

			MP3_Deinit();
			osThreadExit();
			osDelay(1000000);
			continue;
		}

		// поиск синхрослова - начала фрейма
		int offset = MP3FindSyncWord(mp3DecoderState->inputDataPtr, mp3DecoderState->bytes_left);
		if (offset < 0)
		{
			rtt.warning("синхро не найдено 1");
			// синхро не найдено, очистить буфер и прочитать из файла следующую порцию
			mp3DecoderState->bytes_left = 0;
			StopTimeMeasurement();
			continue;
		}

		// указатель на начало фрейма
		mp3DecoderState->inputDataPtr += offset;
		// пропустить ненужные данные в буфере (если есть)
		mp3DecoderState->bytes_left -= offset;

		// проверка валидности фрейма (тип, версия)
		int err = MP3GetNextFrameInfo(mp3DecoderState->hMP3Decoder, &mp3DecoderState->mp3FrameInfo, mp3DecoderState->inputDataPtr);
		if (err < 0
#ifndef MONO_SUPPORT
			|| mp3DecoderState->mp3FrameInfo.nChans != 2	// работаем пока только со стерео
#endif
			|| mp3DecoderState->mp3FrameInfo.layer != 3)
		{
			// поиск следующего фрейма
			mp3DecoderState->bytes_left -= 2;
			mp3DecoderState->inputDataPtr += 2;

			offset = MP3FindSyncWord(mp3DecoderState->inputDataPtr, mp3DecoderState->bytes_left);
			if (offset < 0)
			{
				rtt.warning("синхро не найдено 2");
				// следующее синхро не найдено, очистить буфер
				mp3DecoderState->bytes_left = 0;
				StopTimeMeasurement();
				mp3DecoderState->frameCNT++;
				continue;
			}
			else
			{
				// найден следующий фрейм, повторить цикл
				mp3DecoderState->inputDataPtr += offset;
				mp3DecoderState->bytes_left -= offset;
				StopTimeMeasurement();
				mp3DecoderState->frameCNT++;
				continue;
			}

		}

		short * outbuf;

		if (init == false)
		{
			mp3DecoderState->outBuffPtr = DMA1_Stream5->CR & DMA_SxCR_CT ? 1: 0;
			outbuf = outBuff[mp3DecoderState->outBuffPtr];
		}
		else
		{
			// выбрать свободный буфер для декодированных данных
			outbuf = outBuff[mp3DecoderState->outBuffPtr];
			mp3DecoderState->outBuffPtr = mp3DecoderState->outBuffPtr ? 0: 1;
		}

		// декодирование фрейма
		uint32_t temp = GetCurrentTime();
		err = MP3Decode(mp3DecoderState->hMP3Decoder, &mp3DecoderState->inputDataPtr, (int *)&mp3DecoderState->bytes_left, outbuf, 0);
		temp = GetCurrentTime() - temp;
		mp3DecoderState->summaryDecodeTime_mks += temp;
		if (err < 0)
		{

			playerInfo.error++; //Счетчик ошибок

			if (debug_mode.showDecoderInfo)
			  rtt.error("Ошибка (%d) декодирования фрейма %u, адрес чтения %u\n", (signed int)err,
					  (unsigned int)mp3DecoderState->frameCNT,
					  (unsigned int)mp3DecoderState->fileAddr);

			StopTimeMeasurement();

			// пропустить на 1 байт, чтобы не зависнуть на ошибочном фрейме
			// (если MP3Decode не заберёт ни одного байта из буфера inputBuf)
			if (mp3DecoderState->bytes_left > 0)	{ mp3DecoderState->bytes_left--; mp3DecoderState->inputDataPtr++; 	}
			mp3DecoderState->frameCNT++;
			continue;	// break;	- ошибка!
		}

		// обновить информацию о декодированном фрейме
		// нужно знать, сколько сэмплов декодировано и определить текущий битрейт
		MP3GetLastFrameInfo(mp3DecoderState->hMP3Decoder, &mp3DecoderState->mp3FrameInfo);

		// обновить частоту дискретизации
		if (mp3DecoderState->samprate != mp3DecoderState->mp3FrameInfo.samprate)
		{
			rtt.info("----------");
			rtt.info("Битрейт %d", mp3DecoderState->mp3FrameInfo.bitrate);
			rtt.info("Битность %d", mp3DecoderState->mp3FrameInfo.bitsPerSample);
			rtt.info("outputSamps %d", mp3DecoderState->mp3FrameInfo.outputSamps);
			rtt.info("Каналов %d", mp3DecoderState->mp3FrameInfo.nChans);
			rtt.info("Set SampleRate %d",mp3DecoderState->mp3FrameInfo.samprate);
			rtt.info("layer %d",mp3DecoderState->mp3FrameInfo.layer);
			rtt.info("version %d",mp3DecoderState->mp3FrameInfo.version);
			DAC_setSampleRate(mp3DecoderState->mp3FrameInfo.samprate);
			mp3DecoderState->samprate = mp3DecoderState->mp3FrameInfo.samprate;
		}



		// преобразовать данные в формат, понятный ЦАПу
#ifdef MONO_SUPPORT
		if (mp3DecoderState->mp3FrameInfo.nChans == 2)
		{
			uint32_t outputSamps = mp3DecoderState->mp3FrameInfo.outputSamps;
			//Нормальный режим
			if (outputSamps == 2304)
			{
			 // для стереомузыки только сместить шкалу
			 for (uint32_t i = 0; i < outputSamps; i++)
			 {
				float f = ( outbuf[i])/65536.0F;

		 		int16_t sample = 2047 + (int16_t)(2047.0F  * mp3DecoderState->gain * f);
		 		if (sample > 4095)
		 		{
		 			sample = 4095;
		 			mp3DecoderState->gain -= 0.1F;
		 		}
		 		if (sample < 0){
		 			sample = 0;
		 			mp3DecoderState->gain -= 0.1F;
		 		}
		  		outbuf[i] = sample;
			 }
			}else
			//Каличный режим
			if (outputSamps == 1152)
			{
				 // для стереомузыки только сместить шкалу
				 for (uint32_t i = 0; i < outputSamps * 2; i+=2)
				 {
					float f = ( outbuf[i] )/65536.0F;
			 		int16_t sample = 2048 + (int16_t)(2047.0F * mp3DecoderState->gain * f );
			 		if (sample > 4095)
			 		{
			 		  sample = 4095;
			 		  mp3DecoderState->gain -= 0.1F;
			 		}
			 		if (sample < 0){
			 		  sample = 0;
			 		  mp3DecoderState->gain -= 0.1F;
			 		}
			  		outbuf[i]   = sample;
			  		outbuf[i+1] = sample;
				 }
			}


		}
		else
		{
			uint32_t outputSamps = mp3DecoderState->mp3FrameInfo.outputSamps;
			if (outputSamps == 1152)
			{
				// для моно собрать семплы для левого и правого каналов из моно-сигнала
				short * sptr = &outbuf[mp3DecoderState->mp3FrameInfo.outputSamps - 1];
				short * dptr = &outbuf[mp3DecoderState->mp3FrameInfo.outputSamps*2 - 1];
				for (uint32_t i = 0; i < outputSamps; i++)
				{
					float f = (*sptr--)/65536.0F;
					int16_t sample = 2048 + (int16_t)(2047.0F * mp3DecoderState->gain * f);
			 		if (sample > 4095) {
			 		  sample = 4095;
			 		  mp3DecoderState->gain -= 0.1F;
			 		}
			 		if (sample < 0) {
			 		  sample = 0;
			 		  mp3DecoderState->gain -= 0.1F;
			 		}
					*dptr-- = sample ;//+ 0x8000;
					*dptr-- = sample ;//+ 0x8000;
				}
			}
			else
			if (outputSamps == 576)
			{
				// для моно собрать семплы для левого и правого каналов из моно-сигнала
				short * sptr = &outbuf[mp3DecoderState->mp3FrameInfo.outputSamps - 1];
				short * dptr = &outbuf[mp3DecoderState->mp3FrameInfo.outputSamps*4 - 1];
				for (uint32_t i = 0; i < outputSamps; i++)
				{
					float f = (*sptr--)/65536.0F;
					int16_t sample = 2048 + (int16_t)(2047.0F * mp3DecoderState->gain * f);
			 		if (sample > 4095) {
			 		  sample = 4095;
			 		  mp3DecoderState->gain -= 0.1F;
			 		}
			 		if (sample < 0) {
			 		  sample = 0;
			 		  mp3DecoderState->gain -= 0.1F;
			 		}
					*dptr-- = sample ;//+ 0x8000;
					*dptr-- = sample ;//+ 0x8000;
					*dptr-- = sample ;//+ 0x8000;
					*dptr-- = sample ;//+ 0x8000;
				}
			}

		}

#else
		for (uint32_t i = 0; i < mp3DecoderState->mp3FrameInfo.outputSamps; i++)
		{
			outbuf[i] += 0x8000;	// или проинвертировать старший бит, что то же самое
		}
#endif

		// отправить message ЦАПу
		//uint32_t len = mp3FrameInfo.outputSamps / mp3FrameInfo.nChans;	// делим на 2 канала
		//DAC_interface->SendData((uint32_t *)outbuf, len);

		mp3DecoderState->frameCNT++;
		uint32_t time = StopTimeMeasurement();

		if (debug_mode.showDecoderInfo && debug_mode.showFrameDecodeTime)
		{
			//printf("%u,%u\r\n", (unsigned int)mp3DecoderState->frameCNT, (unsigned int)time/100);
			// отобразить реальное время декодирования
			rtt.print("%u,%u\n", (unsigned int)mp3DecoderState->frameCNT, (unsigned int)temp/100);
		}
		if (decodeStatistic.minCPUidle_mks > time)			decodeStatistic.minCPUidle_mks = time;
		if (decodeStatistic.maxCPUidle_mks < time)			decodeStatistic.maxCPUidle_mks = time;
		decodeStatistic.averageCPUidle_mks += time;

		if((init_count >= 2) && (init)){
			init = false;
		    DAC_DMA_Play();
		}

		//osDelay(1);

	}

	// аварийное завершение задачи недопустимо
	rtt.print("Аварийное завершение задачи mp3Task\r\n");
	stopError();
}


/*************************************************************************************************
 * @brief	Останов декодера
 *
 * @param	player_cmd - команда, отправляемая задаче-проигрывателю
 ************************************************************************************************/
static void stop_decode(cmd_t player_cmd)
{
	// остановить ЦАП
	//mp3DecoderState->DAC_interface->Cmd(DAC_STOP);

	// команда проигрывателю
	player_cmd = player_cmd;
	//CoPostMail(player_CmdMailBox, &player_cmd);
}

static void MP3_Deinit(void)
{
	MP3FreeDecoder(mp3DecoderState->hMP3Decoder);
	free(mp3DecoderState);
	char str[32];
	sprintf(str,"MP3_Deinit: %d",xPortGetFreeHeapSize());
	rtt.colorStringln(0, 183, str);

}




//FreeRTOS
void StartTaskMP3(void *argument)
{
  //Получаем имя запускаемого файла
  rtt.successful("!!!Таска MP4 Запуск");
  MP3((char*)argument);
  for(;;)
  {
    osDelay(999);
  }
}




void play(char * name)
{
	rtt.info("play %s", name);
	rtt.info ("Свободно памяти play %d", xPortGetFreeHeapSize());

	if (myTaskMP3Handle == NULL)
	{
		//osThreadState_t state = osThreadGetState (myTaskMP3Handle);
		//rtt.info("play NULL state %d", state);
		myTaskMP3Handle = osThreadNew(StartTaskMP3, (char *)name, &myTaskMP3_attributes);
	}
	else
	{

		osThreadState_t state = osThreadGetState (myTaskMP3Handle);
	    switch(state){
	      case osThreadInactive : rtt.warning("state osThreadInactive"); break;
	      case osThreadReady : rtt.warning("state osThreadReady"); break;
	      case osThreadRunning : rtt.warning("state osThreadRunning"); break;
	      case osThreadBlocked : rtt.warning("state osThreadBlocked"); break;
	      case osThreadTerminated : rtt.warning("state osThreadTerminated"); break;
	      case osThreadError : rtt.warning("state osThreadError"); break;
	      case osThreadReserved : rtt.warning("state osThreadReserved"); break;
	    }
		rtt.info("play state %d", state);

		//Ждем завершения

		taskMP3_terminate = true;
		while(osThreadGetState (myTaskMP3Handle) != osThreadTerminated)
		{osDelay(1);}

		osDelay(50);
		myTaskMP3Handle = osThreadNew(StartTaskMP3, (char *)name, &myTaskMP3_attributes);
		rtt.info ("Свободно play запуск потока %d", xPortGetFreeHeapSize());
	}







	//if (state == 4)
	//	myTaskMP4Handle = osThreadNew(StartTaskMP4, NULL, &myTaskMP4_attributes);


}
