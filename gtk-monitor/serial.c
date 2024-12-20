#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/time.h>

static int serial_port = -1;
static int freedom_pipe[2]; // I am a clansman!
static int hate_pipe[2];
static struct pollfd fds[2];

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

    if (pipe(freedom_pipe) == -1) {
        fprintf(stderr, "Failed to create freedom pipe: %s\n", strerror(errno));
        close(serial_port);
        return -1;
    }

    if (pipe(hate_pipe) == -1) {
        fprintf(stderr, "Failed to create freedom pipe: %s\n", strerror(errno));
        close(freedom_pipe[0]);
        close(freedom_pipe[1]);
        close(serial_port);
        return -1;
    }
    
    fds[0].fd = freedom_pipe[0];
    fds[0].events = POLLIN;
    fds[1].fd = serial_port;
    fds[1].events = POLLIN;

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
    ret = poll(fds, 2, -1);
    if (ret < 0) {
        fprintf(stderr, "Failed to read (poll): %s\n", strerror(errno));
        return ret;
    }

    // Awaken from read
    if (fds[0].revents & POLLIN) {
        // Notify i'm ready
        if (write(hate_pipe[1], "I HATE U", 8) < 0) {
            fprintf(stderr, "Failed to wake up: %s\n", strerror(errno));
        }
        return -1;
    }

    ret = read(serial_port, msg, length);
    if (ret < 0) {
        fprintf(stderr, "Failed to read: %s\n", strerror(errno));
    }
    return ret;
}

void SERIAL_close()
{
    if (serial_port == -1)
        return;
    
    // Awaken read
    if (write(freedom_pipe[1], "WAKE UP!", 8) < 0) {
        fprintf(stderr, "Failed to wake up: %s\n", strerror(errno));
    }

    // Wait to notification
    char buff[8];
    if (read(hate_pipe[0], buff, 8) < 0) {
        fprintf(stderr, "Failed to wake up: %s\n", strerror(errno));
    }

    // Now we can go
    close(hate_pipe[0]);
    close(hate_pipe[1]);
    close(freedom_pipe[0]);
    close(freedom_pipe[1]);
    close(serial_port);
}


#include <stdint.h>
#include <pthread.h>
#include <signal.h>


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

    double temperature, humidity, light = 0;

    while (!end_program())
    {        
        total_bytes = 0;
        while (total_bytes < sizeof(data_in) && !end_program())
        {
            bytes = SERIAL_read(&buff[total_bytes], sizeof(data_out) - total_bytes);
            if (bytes == -1 && end_program()) {
                goto exit;
            }

            if (bytes > 0)
                total_bytes = total_bytes + bytes;
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
        
        printf("\rHumedad: %15f; Temperatura: %15f; Light: %15f", humidity, temperature, light);
        fflush(stdout);
        
    }

exit:
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
            
            printf("data: %d\n", data_out.data);

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
    SERIAL_close(); 

    // Awake for reader thread to end
    void *ret_val;
    pthread_join(reader_thread, &ret_val);

    return EXIT_SUCCESS;
}