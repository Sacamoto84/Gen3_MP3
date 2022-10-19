#include "main.h"
#include "TFT.h"
#include "dma.h"

extern DMA_HandleTypeDef hdma_spi1_tx;
extern TFT tft CCMRAM;

//DMA SPI1 TX
extern "C" void DMA2_Stream5_IRQHandler(void)
{
	if (DMA2->HISR & DMA_HISR_TCIF5)
		      tft.DMA_TX_Complete = 1;

  HAL_DMA_IRQHandler(&hdma_spi1_tx);
}
