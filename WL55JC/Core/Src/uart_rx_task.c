/*
 * uart_rx_task.c
 *
 *  Created on: Dec 20, 2024
 *      Author: 1218382
 */

#include "uart_msg.h"
#include "util.h"
#include "leds.h"

static struct UART_Msg msg;

void uart_rx_task_pre_loop()
{
	HAL_UART_Receive_IT(&huart2, (uint8_t *)&msg, sizeof(msg));

	HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
}

void uart_rx_task_iteration()
{
	// Await interruption flag
	osEventFlagsWait(event_flagsHandle, EVENT_FLAG_UART_RX_DONE, osFlagsWaitAll, osWaitForever);

	// Make action
	switch (msg.cmd)
	{
	case CMD_SERVO:
		htim16.Instance->CCR1 = msg.data < 700 ? 700 : msg.data > 3300 ? 3300 : msg.data;
		break;
	case CMD_LED_ON:
		LED_on(msg.data);
		break;
	case CMD_LED_OFF:
		LED_off(msg.data);
		break;
	case CMD_LED_TOGGLE:
		LED_toggle(msg.data);
		break;
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	osEventFlagsSet(event_flagsHandle, EVENT_FLAG_UART_RX_DONE);
	// Always try to receive something
	HAL_UART_Receive_IT(&huart2, (uint8_t *)&msg, sizeof(msg));
}


