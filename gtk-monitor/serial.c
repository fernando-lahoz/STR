#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

static int serial_port = -1;

int SERIAL_open(const char *dev_name)
{
    serial_port = open(dev_name, O_RDWR); // Change to the appropriate serial port
    if (serial_port < 0) {
        fprintf(stderr, "Failed to open the serial port: %s\n", strerror(errno));
        return -1;
    }

    struct termios tty;
    if (tcgetattr(serial_port, &tty) != 0) {
        fprintf(stderr, "Failed to get attributes: %s\n", strerror(errno));
        close(serial_port);
        return -1;
    }

    // Configure the serial port
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK; // Disable break processing
    tty.c_lflag = 0; // No signaling chars, no echo, no canonical processing
    tty.c_oflag = 0; // No remapping, no delays
    tty.c_cc[VMIN] = 1; // Read blocks
    tty.c_cc[VTIME] = 1; // Read timeout (tenths of a second)
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Shut off XON/XOFF ctrl
    tty.c_cflag |= (CLOCAL | CREAD); // Ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD); // Disable parity
    tty.c_cflag &= ~CSTOPB; // 1 stop bit
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS flow control
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Failed to set attributes: %s\n", strerror(errno));
        close(serial_port);
        return -1;
    }

    return 0;
}

ssize_t SERIAL_write(const void* msg, size_t length)
{
    ssize_t ret;
    ret = write(serial_port, msg, length);
    if (ret < 0) {
        fprintf(stderr, "Failed to write: %s\n", strerror(errno));
    }
    return ret;
}

ssize_t SERIAL_read(void *msg, size_t length)
{
    ssize_t ret;
    ret = read(serial_port, msg, length);
    if (ret < 0) {
        fprintf(stderr, "Failed to read: %s\n", strerror(errno));
    }
    return ret;
}

void SERIAL_close()
{
    if (serial_port != -1)
        close(serial_port);
}


#include <stdint.h>
#include <pthread.h>

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
int end = 0;

enum {
    CMD_TEMPERATURE,
    CMD_HUMIDITY,
    CMD_LIGHT,
    CMD_SERVO
};

struct msg {
    uint32_t cmd;
    uint32_t data;
};

struct msg data_in, data_out;

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

    double temperature, humidity;

    printf("Thread created\n");

    while (!end_program())
    {        
        total_bytes = 0;
        while (total_bytes < sizeof(data_in) && !end_program())
        {
            bytes = SERIAL_read(&buff[total_bytes], sizeof(data_out) - total_bytes);
            //printf("bytes: %ld\n", bytes);
            total_bytes = bytes < 0 ? sizeof(data_in) : total_bytes + bytes;
        }

        memcpy(&data_in, buff, sizeof(data_in));
        
        if (data_in.cmd == CMD_HUMIDITY)
        {
            humidity = (double)((int16_t)data_in.data) / 2.0;
            
        }
        else if (data_in.cmd == CMD_TEMPERATURE)
        {
            temperature = (double)((int16_t)data_in.data) / 8.0;
            
        }
        printf("\rHumedad: %15f; Temperatura: %15f;", humidity, temperature);
        
    }
    pthread_mutex_unlock(&mtx);

    printf("Thread exit\n");
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

    while (!end)
    {
        printf(" # ");
        scanf("%s %s", type, msg);

        if (strcmp(type, "HUMIDITY") == 0)
        {
            data_out.cmd = CMD_HUMIDITY;
            data_out.data = atoi(msg);
            SERIAL_write(&data_out, sizeof(data_out));
            /*
            ssize_t bytes = SERIAL_read(&data_in, sizeof(data_in));
            printf("bytes: %ld;  cmd: %d; data: %d\n", bytes, data_in.cmd, data_in.data);
            if (data_in.cmd == CMD_HUMIDITY)
            {
                printf("Humedad: %d", data_in.data);
            }
            */
        }
        else if (strcmp(type, "END") == 0)
        {
            pthread_mutex_lock(&mtx);
            end = 1;
            pthread_mutex_unlock(&mtx);
        }
        else
        {
            printf("Unrecognized command\n");
        }
    }

    SERIAL_close();
    return EXIT_SUCCESS;
}