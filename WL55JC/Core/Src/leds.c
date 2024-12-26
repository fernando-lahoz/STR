#include "leds.h"
#include "util.h"
#include "stm32wlxx_hal.h"

static typeof(GPIOB) led_gpio[] = {
	[LED_BOARD_GREEN] = GPIOB,
	[LED_BOARD_RED] = GPIOB,
	[LED_BOARD_BLUE] = GPIOB,
	// TODO: config other 4 leds
};

static typeof(GPIO_PIN_15) led_pin[] = {
	[LED_BOARD_GREEN] = GPIO_PIN_9,
	[LED_BOARD_RED] = GPIO_PIN_11,
	[LED_BOARD_BLUE] = GPIO_PIN_15,
	// TODO: config other 4 leds
};

void LED_on(enum LED_Color led)
{
	if (led < 3)
		HAL_GPIO_WritePin(led_gpio[led], led_pin[led], 1);
}

void LED_off(enum LED_Color led)
{
	if (led < 3)
		HAL_GPIO_WritePin(led_gpio[led], led_pin[led], 0);
}

void LED_toggle(enum LED_Color led)
{
	if (led < 3)
		HAL_GPIO_TogglePin(led_gpio[led], led_pin[led]);
}

