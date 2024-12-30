#include "util.h"
#include "uart_msg.h"

#include "leds.h"

static uint32_t value;

void light_adc_task_pre_loop()
{
	HAL_ADC_Start_IT(&hadc);
}

void light_adc_task_iteration()
{
	struct UART_Msg msg;
	HAL_ADC_Start_IT(&hadc);


	msg.cmd = CMD_LIGHT;
	msg.data = value;
	osMessageQueuePut(serial_tx_data_queueHandle, &msg, 0, osWaitForever);
	// Await for any sensor data to be ready
	osEventFlagsWait(event_flagsHandle, EVENT_FLAG_ADC_CONV_DONE, osFlagsWaitAll, osWaitForever);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	value = HAL_ADC_GetValue(hadc);

	osEventFlagsSet(event_flagsHandle, EVENT_FLAG_ADC_CONV_DONE);
}
