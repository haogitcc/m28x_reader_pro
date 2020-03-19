#ifndef _GPIO_INIT_H_
#define _GPIO_INIT_H_

#define GPO_28 44
#define GPO_29 43
#define GPI_30 42
#define GPI_31 41

//#define GPO1_DEVICE "/sys/class/gpio/gpio108/value"
#define GPO2_DEVICE "/sys/class/gpio/gpio44/value"
#define GPO3_DEVICE "/sys/class/gpio/gpio43/value"
#define GPO4_DEVICE "/sys/class/gpio/gpio42/value"
#define GPO5_DEVICE "/sys/class/gpio/gpio41/value"

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
