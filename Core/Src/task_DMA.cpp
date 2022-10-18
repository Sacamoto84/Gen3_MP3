#include "global.h"
#include "mp3dec.h"
#include "fatfs.h"
#include "playerTask.h"
#include "debugTask.h"
#include "main.h"

extern DMA_HandleTypeDef hdma_dac1;

#define OUTPUTSAMPLES 1

uint32_t OUTPUT[OUTPUTSAMPLES];

#define DAC_BUFFER_SIZE		(1152*2)
int16_t   outBuff[2][1152*2] RAM_16; // буфер выходного потока

uint DMA_Buffer_Current;

void init_MP3_DAC_DMA(void)
{
	/**************************
	 *  DMA DAC
	 **************************/
	DAC1->CR |= DAC_CR_DMAEN1 | DAC_CR_EN1 | DAC_CR_DMAEN2 | DAC_CR_EN2;
	//HAL_DMA_Start_IT(&hdma_dac1, 0x2001C000, 0x40007420, 4096);
	DMA1_Stream5->CR &= ~DMA_SxCR_EN; //Выключаем DMA
	DMA1_Stream5->NDTR = 1152;
	DMA1_Stream5->PAR  = 0x40007420; //DAC 12R

	DMA1_Stream5->M0AR = (uint32_t) &outBuff[1][0]; //(int)pOUTPUT;
	DMA1_Stream5->M1AR = (uint32_t) &outBuff[0][0]; //(int)pOUTPUT;

	//DMA1_Stream5->CR |=  DMA_SxCR_CIRC;// | DMA_SxCR_HTIE;
	////DMA1_Stream5->CR &= ~DMA_SxCR_TCIE;
	// DMA_SxCR_HTIE; //Прерывание | Циклический режим | //Двойной буффер
	DMA1_Stream5->CR |= DMA_SxCR_TCIE | DMA_SxCR_CIRC | DMA_SxCR_DBM;
	DMA1_Stream5->CR |= DMA_SxCR_EN;

	//HAL_TIM_Base_Start(&htim6);  //Запуск таймера6 для DAC
}


//Очистить буффер
void DAC_DMA_ClearBuffer(void)
{
	int16_t *p = &outBuff[0][0];
	for (int i = 0 ; i < 9216/2 ; i++)
	{
		*p = 2047;
		p++;
	}
}





void DAC_DMA_Pause(void)
{
  HAL_TIM_Base_Stop(&htim6);
}

void DAC_DMA_Play(void)
{
	HAL_TIM_Base_Start(&htim6);
}

void DAC_setSampleRate(int sample = 44000)
{
	uint16_t period = 108000000/sample;
	period--;

	TIM6->CNT = 0;
	TIM6->ARR = period;
}

traceHandle Timer1Handle = xTraceSetISRProperties("ISR DAC DMA", 5);

extern "C" void DMA1_Stream5_IRQHandler(void)
{

	vTraceStoreISRBegin(Timer1Handle);
	if (DMA1_Stream5->CR & DMA_SxCR_CT)
	{
		DMA_Buffer_Current = 1;
	}
	else
	{
		DMA_Buffer_Current = 2;
	}

	HAL_DMA_IRQHandler(&hdma_dac1);
	vTraceStoreISREnd(0);
}
