#include "util.h"
#include "uart_msg.h"

#include "HTS221.h"

#include "leds.h"

void sensor_i2c_task_pre_loop()
{
	HTS221_configure(HTS221_TEMP_AVG_2, HTS221_HUM_AVG_4, HTS221_ODR_1HZ);
	HTS221_start();
}

void sensor_i2c_task_iteration()
{
	struct UART_Msg msg;
	int16_t temperature_x8, humdity_x2;

	// Check if humidity and temperature are ready
	if (HTS221_is_ready()) {
		HTS221_get_data(&temperature_x8, &humdity_x2);

		msg.cmd = CMD_HUMIDITY;
		msg.data = humdity_x2;
		osMessageQueuePut(serial_tx_data_queueHandle, &msg, 0, osWaitForever);
		msg.cmd = CMD_TEMPERATURE;
		msg.data = temperature_x8;
		osMessageQueuePut(serial_tx_data_queueHandle, &msg, 0, osWaitForever);
	}

	// Check if nano-pressure is ready
	// ...

	// Await for any sensor data to be ready
	osEventFlagsWait(event_flagsHandle, EVENT_FLAG_USER_INTERRUPT, osFlagsWaitAll, osWaitForever);
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	osEventFlagsSet(event_flagsHandle, EVENT_FLAG_I2C_MEM_RX_DONE);
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	osEventFlagsSet(event_flagsHandle, EVENT_FLAG_I2C_MEM_TX_DONE);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == GPIO_PIN_12)
		osEventFlagsSet(event_flagsHandle, EVENT_FLAG_USER_INTERRUPT);
}


