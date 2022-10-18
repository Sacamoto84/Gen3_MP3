/*
 * debugTask.h
 *
 *  Created on: 12 окт. 2022 г.
 *      Author: Ivan
 */

#ifndef INC_DEBUGTASK_H_
#define INC_DEBUGTASK_H_

#include "main.h"
#include "tim.h"

/// структура с отлалочными флагами
typedef struct
{
	uint32_t timer_ms;
	_Bool showDebugInfo;
	_Bool showDecoderInfo;
	_Bool showFrameDecodeTime;
	_Bool showPlayerInfo;
} debug_mode_t;

/// структура для сбора статистики работы
typedef struct
{
	// описание декодера
	uint32_t fileNumber;		// номер текущего файла
	char fileName[13];			// имя текущего файла
	//fileTypes_t fileType;		// тип текущего файла

	// загрузка CPU
	uint32_t minCPUidle_mks;	// минимальная загрузка CPU за 1 фрейм
	uint32_t maxCPUidle_mks;	// максимальная загрузка CPU за 1 фрейм
	uint32_t averageCPUidle_mks;// накопитель времени для расчёта средней загрузки CPU за 1 трек

	// ошибки
	uint32_t dacBuffLoss;		// счётчик количества потерь синхронизации ЦАП и декодера (данные не готовы)
} decodeStatistic_t;

extern debug_mode_t debug_mode;	// отладочные флаги
extern decodeStatistic_t decodeStatistic;

// функция-маркер начала измеряемого временного интервала
static inline void StartTimeMeasurement()
{
	TIM12->CR1 &= (uint16_t)~TIM_CR1_CEN;
	debug_mode.timer_ms = 0;
	TIM12->CNT = 0;
	TIM12->CR1 |= TIM_CR1_CEN;
}

// останов таймера и возврат времени с момента вызова StartTimeMeasurement()
static inline uint32_t StopTimeMeasurement()
{
	TIM12->CR1 &= (uint16_t)~TIM_CR1_CEN;
	return debug_mode.timer_ms*1000 + TIM12->CNT;
}

// функция-маркер окончания измеряемого временного интервала
// возвращает время между вызовами start_timer() и stop_timer() в мкс
static inline uint32_t GetCurrentTime()
{
	__disable_irq();
	uint32_t temp = debug_mode.timer_ms*1000 + TIM12->CNT;
	__enable_irq();
	return temp;
}


#endif /* INC_DEBUGTASK_H_ */
