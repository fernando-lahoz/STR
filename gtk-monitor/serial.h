#ifndef GTK_MONITOR_SERIAL_H
#define GTK_MONITOR_SERIAL_H

#include <stdlib.h>

int SERIAL_open(const char *dev_name);

ssize_t SERIAL_write(const void* msg, size_t length);

ssize_t SERIAL_read(void *msg, size_t length);

void SERIAL_wake_up();

void SERIAL_close();

#endif