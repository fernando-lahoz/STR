#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/eventfd.h>

#include "serial.h"

static int serial_port = -1, wakeup_event;
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

    wakeup_event = eventfd(0, 0);
    if (wakeup_event == -1) {
        fprintf(stderr, "Failed to create eventfd: %s\n", strerror(errno));
        close(serial_port);
        return -1;
    }
    
    fds[0].fd = wakeup_event;
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
    ret = poll(fds, 1, -1);
    if (ret < 0) {
        fprintf(stderr, "Failed to read (poll): %s\n", strerror(errno));
        return ret;
    }

    // Awaken from read
    if (fds[0].revents & POLLIN) {
        return 0;
    }

    ret = read(serial_port, msg, length);
    if (ret < 0) {
        fprintf(stderr, "Failed to read: %s\n", strerror(errno));
    }
    return ret;
}

void SERIAL_wake_up()
{
    if (serial_port == -1)
        return;
    
    // Awake read
    uint64_t buff = 1;
    if (write(wakeup_event, &buff, 8) < 0) {
        fprintf(stderr, "Failed to wake up: %s\n", strerror(errno));
    }
}

void SERIAL_close()
{
    if (serial_port == -1)
        return;
    
    close(wakeup_event);
    close(serial_port);
}