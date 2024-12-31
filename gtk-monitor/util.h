#ifndef GTK_MONITOR_UTIL_H
#define GTK_MONITOR_UTIL_H

#include <stdint.h>
#include <gtk/gtk.h>
#include "plot.h"

enum UART_Cmd {
    CMD_TEMPERATURE,
    CMD_HUMIDITY,
    CMD_LIGHT,
    CMD_PREASSURE,

    CMD_SERVO,
    CMD_LED_ON,
    CMD_LED_OFF,
    CMD_LED_TOGGLE
};

struct msg {
    uint32_t cmd;
    uint32_t data;
};

enum LED_Color {
	LED_BOARD_GREEN,
	LED_BOARD_RED,
	LED_BOARD_BLUE,

	LED_GREEN,
	LED_RED,
	LED_BLUE,
	LED_YELLOW
};

enum {
    INPUT_HUMIDITY,
    INPUT_TEMPERATURE,
    INPUT_LIGHT,

    INPUT_SIZE
};

extern const char* input_name[];

typedef struct input_sensor {
    struct {
        GtkWidget* widget;
        char *file_name;
    } picture;
    PLOT_graph_t* graph;
    
} input_sensor_t;

extern input_sensor_t input[INPUT_SIZE];

#define DEFAULT_WINDOWS_SIZE (20)
#define DEFAULT_COLOR (0xFFFFFF80)

void reload_img(void* data);

int launch_reader_thread(const char* dev);
int terminate_reader_thread(void);

#endif