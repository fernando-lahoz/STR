/*
 * uart_msg.h
 *
 *  Created on: Dec 20, 2024
 *      Author: 1218382
 */

#ifndef UART_MSG_H
#define UART_MSG_H

#include "util.h"

enum UART_Cmd {
	// Tx commands
    CMD_TEMPERATURE,
    CMD_HUMIDITY,
    CMD_LIGHT,
	CMD_PREASSURE,

	// Rx commands
    CMD_SERVO,
	CMD_LED_ON,
	CMD_LED_OFF,
	CMD_LED_TOGGLE
};

struct UART_Msg {
    uint32_t cmd;
    uint32_t data;
};

#endif
