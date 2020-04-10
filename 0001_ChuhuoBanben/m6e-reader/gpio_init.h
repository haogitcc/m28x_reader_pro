#ifndef _GPIO_INIT_H_
#define _GPIO_INIT_H_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <poll.h>

#include "app_sys_setting.h"
#include "reader.h"

#define MSG(args...) printf(args);

static int fdo1,fdo2,fdi;
static struct timeval tv;

void gpio_init();
int gpio_export(int pin);
int gpio_unexport(int pin);
int gpio_direction(int pin, int dir);
int gpio_write(int pin, int value);
int gpio_read(int pin);
int gpio_edge(int pin, int edge);
int gpio_poll();
int gpio_create();
int gpio_write_60(int timeout);
int gpio_write_61(int timeout);
int gpio_write_60_v(char *str);
int gpio_write_61_v( char *str);


#endif
