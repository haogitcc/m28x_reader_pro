#ifndef _GPIO_INIT_H_
#define _GPIO_INIT_H_


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
void gpio_stop();

#endif
