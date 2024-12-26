/*
 * util.h
 *
 *  Created on: Dec 20, 2024
 *      Author: 1218382
 */

#ifndef UTIL_H
#define UTIL_H

#include "stm32wlxx_hal.h"
#include "cmsis_os.h"

extern ADC_HandleTypeDef hadc;
extern I2C_HandleTypeDef hi2c2;
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim16;

extern osMessageQueueId_t serial_tx_data_queueHandle;

#define BIT(n) (1 << n)

enum Event_Flags {
	EVENT_FLAG_UART_TX_DONE = BIT(0),
	EVENT_FLAG_UART_RX_DONE = BIT(1),
	EVENT_FLAG_I2C_MEM_TX_DONE = BIT(2),
	EVENT_FLAG_I2C_MEM_RX_DONE = BIT(3),
	EVENT_FLAG_USER_INTERRUPT = BIT(4),
	EVENT_FLAG_ADC_CONV_DONE = BIT(5)
};

extern osEventFlagsId_t event_flagsHandle;

#endif
