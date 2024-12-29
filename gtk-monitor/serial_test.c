#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "serial.h"

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
int end = 0;

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

struct msg data_in, data_out;

enum LED_Color {
	LED_BOARD_GREEN,
	LED_BOARD_RED,
	LED_BOARD_BLUE,

	LED_GREEN,
	LED_RED,
	LED_BLUE,
	LED_YELLOW
};



int end_program()
{
    int ret;
    pthread_mutex_lock(&mtx);
    ret = end;
    pthread_mutex_unlock(&mtx);
    return ret;
}

void *reader(void *)
{
    ssize_t bytes, total_bytes;
    uint8_t buff[sizeof(data_in)];

    double temperature = 0, humidity = 0, light = 0;

    while (!end_program())
    {
        total_bytes = 0;
        while (total_bytes < sizeof(data_in) && !end_program())
        {
            bytes = SERIAL_read(&buff[total_bytes], sizeof(data_out) - total_bytes);
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
            light = (double)(data_in.data);
            break;
        case CMD_HUMIDITY:
            humidity = (double)(data_in.data) / 2.0;
            break;
        case CMD_TEMPERATURE:
            temperature = (double)(data_in.data) / 8.0;
            break;
        }
        
        //printf("\rHumedad: %15f; Temperatura: %15f; Light: %15f", humidity, temperature, light);
        fflush(stdout);
        
    }
    
exit:
    printf("End of thread\n");
    return NULL;
}

int main()
{
    if (SERIAL_open("/dev/ttyACM0"))
        return EXIT_FAILURE;

    pthread_t reader_thread;

    if (pthread_create(&reader_thread, NULL, reader, (void*)&data_in) != 0) {
        printf("Error on pthread_create()\n");
        SERIAL_close();
        return EXIT_FAILURE;
    }

    char type[64], msg[256];

    while (!end_program())
    {
        printf(" # ");
        scanf("%s %s", type, msg);

        if (strcmp(type, "SERVO") == 0)
        {
            data_out.cmd = CMD_SERVO;
            data_out.data = atoi(msg);
            data_out.data = data_out.data < 700 ? 700 : data_out.data > 3300 ? 3300 : data_out.data; 
            SERIAL_write(&data_out, sizeof(data_out));
        }
        else if (strcmp(type, "LED_ON") == 0)
        {
            data_out.cmd = CMD_LED_ON;
            data_out.data = -1;
            if (strcmp(msg, "BOARD_GREEN") == 0)
                data_out.data = LED_BOARD_GREEN;
            else if (strcmp(msg, "BOARD_BLUE") == 0)
                data_out.data = LED_BOARD_BLUE;
            else if (strcmp(msg, "BOARD_RED") == 0)
                data_out.data = LED_BOARD_RED;

            if (data_out.data != -1)
                SERIAL_write(&data_out, sizeof(data_out));
        }
        else if (strcmp(type, "LED_OFF") == 0)
        {
            data_out.cmd = CMD_LED_OFF;
            data_out.data = -1;
            if (strcmp(msg, "BOARD_GREEN") == 0)
                data_out.data = LED_BOARD_GREEN;
            else if (strcmp(msg, "BOARD_BLUE") == 0)
                data_out.data = LED_BOARD_BLUE;
            else if (strcmp(msg, "BOARD_RED") == 0)
                data_out.data = LED_BOARD_RED;

            

            if (data_out.data != -1)
                SERIAL_write(&data_out, sizeof(data_out));
        }
        else if (strcmp(type, "LED_TOGGLE") == 0)
        {
            data_out.cmd = CMD_LED_TOGGLE;
            data_out.data = -1;
            if (strcmp(msg, "BOARD_GREEN") == 0)
                data_out.data = LED_BOARD_GREEN;
            else if (strcmp(msg, "BOARD_BLUE") == 0)
                data_out.data = LED_BOARD_BLUE;
            else if (strcmp(msg, "BOARD_RED") == 0)
                data_out.data = LED_BOARD_RED;

            if (data_out.data != -1)
                SERIAL_write(&data_out, sizeof(data_out));
        }
        else if (strcmp(type, "END") == 0)
        {
            printf("Ending\n");
            pthread_mutex_lock(&mtx);
            end = 1;
            pthread_mutex_unlock(&mtx);
        }
        else
        {
            printf("Unrecognized command\n");
        }
    }

    // Awake blocked functions and close serial
    SERIAL_wake_up(); 

    // Awake for reader thread to end
    void *ret_val;
    pthread_join(reader_thread, &ret_val);

    SERIAL_close();
    return EXIT_SUCCESS;
}