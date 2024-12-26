#ifndef LEDS_H
#define LEDS_H

enum LED_Color {
	LED_BOARD_GREEN,
	LED_BOARD_RED,
	LED_BOARD_BLUE,

	LED_GREEN,
	LED_RED,
	LED_BLUE,
	LED_YELLOW
};

void LED_on(enum LED_Color led);
void LED_off(enum LED_Color led);
void LED_toggle(enum LED_Color led);

#endif
