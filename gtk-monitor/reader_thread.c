#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "serial.h"
#include "util.h"

static struct msg data_in;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static int end = 0;

static int end_program()
{
    int ret;
    pthread_mutex_lock(&mtx);
    ret = end;
    pthread_mutex_unlock(&mtx);
    return ret;
}

static void *reader(void *)
{
    ssize_t bytes, total_bytes;
    uint8_t buff[sizeof(data_in)];

    double humidity, temperature, light;

    while (!end_program())
    {
        total_bytes = 0;
        while (total_bytes < sizeof(data_in) && !end_program())
        {
            bytes = SERIAL_read(&buff[total_bytes], sizeof(data_in) - total_bytes);
            if (bytes > 0)
                total_bytes = total_bytes + bytes;
        }
        if (end_program()) {
            goto exit;
        }

        memcpy(&data_in, buff, sizeof(data_in));

        switch (data_in.cmd)
        {
        case CMD_LIGHT:
            light = 100.0 * (data_in.data - 580) / (4000.0 - 580.0);
            light = light < 0.0 ? 0.0 : light > 100.0 ? 100.0 : light;
            PLOT_push_data(input[INPUT_LIGHT].graph, light);
            PLOT_plot_to_file(input[INPUT_LIGHT].graph, reload_img, (void*)INPUT_LIGHT, 0);
            break;
        case CMD_HUMIDITY:
            humidity = (double)(data_in.data) / 2.0;
            PLOT_push_data(input[INPUT_HUMIDITY].graph, humidity);
            PLOT_plot_to_file(input[INPUT_HUMIDITY].graph, reload_img, (void*)INPUT_HUMIDITY, 0);
            break;
        case CMD_TEMPERATURE:
            temperature = (double)(data_in.data) / 8.0;
            PLOT_push_data(input[INPUT_TEMPERATURE].graph, temperature);
            PLOT_plot_to_file(input[INPUT_TEMPERATURE].graph, reload_img, (void*)INPUT_TEMPERATURE, 0);
            break;
        }
    }
    
exit:
    printf("End of thread\n");
    return NULL;
}

static pthread_t reader_thread;

int launch_reader_thread(const char* dev)
{
    end = 0;

    if (SERIAL_open(dev))
        return EXIT_FAILURE;

    if (pthread_create(&reader_thread, NULL, reader, (void*)&data_in) != 0) {
        printf("Error on pthread_create()\n");
        SERIAL_close();
        return EXIT_FAILURE;
    }
}

int terminate_reader_thread()
{
    pthread_mutex_lock(&mtx);
    end = 1;
    pthread_mutex_unlock(&mtx);

    SERIAL_wake_up(); 
    
    void *ret_val;
    pthread_join(reader_thread, &ret_val);

    SERIAL_close();
}