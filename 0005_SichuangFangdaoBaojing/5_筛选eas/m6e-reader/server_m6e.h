#ifndef _SERVER_M6E_H
#define _SERVER_M6E_H

#ifndef SHUT_RD
#define SHUT_RD 0
#endif
#ifndef SHUT_WR
#define SHUT_WR 1
#endif
#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif

#include <stdint.h>

#define FIRMWARE_M6E
//#define FIRMWARE_MIRCO
//#define FIRMWARE_NANO

#ifdef FIRMWARE_M6E
#define FIRMWARE "1.8.1.13"
#endif

#ifdef FIRMWARE_MIRCO
#define FIRMWARE "1.3.1.1"
#endif

#ifdef FIRMWARE_NANO
#define FIRMWARE "1.16.1.1"
#endif

//#define SQLITE3_ENABLE
#define UPGRADE_ENABLE


typedef struct tagEPC {
	char epc[256];
	int length;
	struct tagEPC *next;
}tagEPC;


int pthread_tag_init();
int m6e_start(void);
int m6e_baudrate(int rate);
int m6e_version();
int m6e_configuration_init();
int tcp_sendBytes(uint32_t length, uint8_t* message);

#endif

