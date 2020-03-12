#ifndef _GPIO_INIT_H_
#define _GPIO_INIT_H_

void gpio_init();
void setTimer(int seconds, int mseconds);
int gpo_open(char *device);
int gpo_close();
int gpo_init();
int gpo1_write();
int gpo2_write();
int gpio_poll();


#endif
