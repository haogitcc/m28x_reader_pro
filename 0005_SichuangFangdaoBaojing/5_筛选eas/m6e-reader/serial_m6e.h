#ifndef _SERIAL_M6E_H
#define _SERIAL_M6E_H

#include <sys/types.h>
#include <stdint.h>

int serial_open(const char *str);
int serial_sendBytes(uint32_t length, uint8_t* message);
int serial_sendBytes(uint32_t length, uint8_t* message);
int serial_setBaudRate(uint32_t rate);
int serial_shutdown();
int serial_flush();
int receiveMessage(uint8_t *message, int *len, int timeoutMs);


#endif
