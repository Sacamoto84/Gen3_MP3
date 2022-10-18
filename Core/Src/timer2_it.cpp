#include "global.h"

//Правый энкодер
extern "C" void TIM2_IRQHandler(void)
{
	if (TIM2->CR1 & TIM_CR1_DIR)
	{  
	  Encoder.Right = 1;
	}
	else{
	  Encoder.Left  = 1;
	}
	
    HAL_TIM_IRQHandler(&htim2);
}
