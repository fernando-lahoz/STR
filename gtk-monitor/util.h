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
    LED_YELLOW,

    LED_NUMBER
};

enum Input {
    INPUT_HUMIDITY,
    INPUT_TEMPERATURE,
    INPUT_LIGHT,

    INPUT_SIZE
};

typedef struct input_attr {
    const char* name;
    const char* title;
    const char* units;
    size_t window_size;
    double min, max;
    uint32_t rgba;
} input_attr_t;

// Constants and default values
extern input_attr_t input_attr[];

typedef struct input_sensor {
    struct {
        GtkWidget* widget;
        char *file_name;
    } picture;
    PLOT_graph_t* graph;
    struct {
        GtkWidget* tab_widget;
        GtkWidget* page_widget;
        double value;
    } info;
    struct {
        GtkWidget* widget;
        GdkRGBA rgba;
    } color;
    uint32_t rgba;
} input_sensor_t;

// Dynamic values
extern input_sensor_t input[INPUT_SIZE];

#define DEFAULT_WINDOWS_SIZE (20)
#define DEFAULT_COLOR (0xFFFFFF80)

enum Output {
    OUTPUT_LED,
    /* space for leds */
    OUTPUT_SERVO = LED_NUMBER,

    OUTPUT_SIZE 
};

void mbox_send(int slot, struct msg* data);

void reload_img(void* data);

int launch_reader_thread(void);
void* terminate_reader_thread(void);

int launch_writer_thread(void);
void* terminate_writer_thread(void);

static inline GdkRGBA u32_to_GdkRGBA(uint32_t rgba)
{
    GdkRGBA res = {
        .alpha = (float)(rgba & 0xFF)/255.0f,
        .blue = (float)((rgba >> 8) & 0xFF)/255.0f,
        .green = (float)((rgba >> 16) & 0xFF)/255.0f,
        .red = (float)((rgba >> 24) & 0xFF)/255.0f,
    };
    return res;
}

static inline uint32_t u32_from_GdkRGBA(const GdkRGBA* rgba)
{
    return ((uint32_t)((uint8_t)(rgba->red * 255)) << 24)
         | ((uint32_t)((uint8_t)(rgba->green * 255)) << 16)
         | ((uint32_t)((uint8_t)(rgba->blue * 255)) << 8)
         | ((uint32_t)(uint8_t)(rgba->alpha * 255));
}

#endif