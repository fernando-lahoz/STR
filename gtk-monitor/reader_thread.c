#define _XOPEN_SOURCE 700 // Posix 2017 in order to use mkdtemp

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

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

static uint8_t sync_bytes[8] = {0xEE, 0x11, 0xCC, 0x33, 0xAA, 0x55, 0x88, 0x77};

static void *reader(void *)
{
    ssize_t bytes, total_bytes;
    uint8_t buff[sizeof(data_in)];

    double humidity, temperature, light;

    gboolean data_is_garbage = 1;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (!end_program())
    {
        total_bytes = 0;
        while (total_bytes < 8)
        {
            bytes = SERIAL_read(&buff[total_bytes], 8 - total_bytes);
        
            if (end_program())
                goto exit;

            if (bytes <= 0) 
                continue;

        restart_check:
            for (ssize_t i = 0; i < bytes; ++i) {
                //printf("total_bytes: %lu,  (0x%X) vs (0x%X)\n", total_bytes, buff[total_bytes], sync_bytes[total_bytes]);
                //system("read -p 'Press ENTER to continue...' var");
                if (buff[total_bytes] != sync_bytes[total_bytes]) {
                    
                    for (size_t j = 0; j < bytes - i - 1; j++) {
                        buff[j] = buff[total_bytes + j + 1];
                    }
                    bytes = bytes - i - 1;
                    total_bytes = 0;
                    goto restart_check;
                }
                total_bytes++;
            }
        }
        
        total_bytes = 0;

        // MSG is correctly aligned -> read it
        while (total_bytes < sizeof(data_in))
        {
            bytes = SERIAL_read(&buff[total_bytes], sizeof(data_in) - total_bytes);
        
            if (end_program())
                goto exit;

            if (bytes > 0) 
                total_bytes = total_bytes + bytes;
        } 

        // Messages received before 200ms of run time are discarded to remove garbage from serial port
        if (data_is_garbage) {
            clock_gettime(CLOCK_MONOTONIC, &end);
            data_is_garbage = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9 <  0.2;
            continue;
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
    return NULL;
}

static pthread_t reader_thread;

int launch_reader_thread()
{
    end = 0;

    if (pthread_create(&reader_thread, NULL, reader, (void*)&data_in) != 0) {
        fprintf(stderr, "Failed to create reader thread: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void* terminate_reader_thread()
{
    pthread_mutex_lock(&mtx);
    end = 1;
    pthread_mutex_unlock(&mtx);

    SERIAL_wake_up(); 
    
    void *ret_val;
    pthread_join(reader_thread, &ret_val);

    return ret_val;
}
