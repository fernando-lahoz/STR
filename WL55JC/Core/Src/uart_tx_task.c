/*
 * uart_tx_task.c
 *
 *  Created on: Dec 20, 2024
 *      Author: 1218382
 */

#include "uart_msg.h"
#include "util.h"

#include "leds.h"

static struct UART_Msg_TX {
	uint8_t sync_bytes[8];
	struct UART_Msg msg;
} msg = { .sync_bytes = {0xEE, 0x11, 0xCC, 0x33, 0xAA, 0x55, 0x88, 0x77} };

void uart_tx_task_iteration()
{
	osMessageQueueGet(serial_tx_data_queueHandle, &msg.msg, NULL, osWaitForever);
	// Wait for Tx message to write
    HAL_UART_Transmit_IT(&huart2, (uint8_t*)&msg, sizeof(msg));

	// Await interruption flag
    osEventFlagsWait(event_flagsHandle, EVENT_FLAG_UART_TX_DONE, osFlagsWaitAll, osWaitForever);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	osEventFlagsSet(event_flagsHandle, EVENT_FLAG_UART_TX_DONE);
}
