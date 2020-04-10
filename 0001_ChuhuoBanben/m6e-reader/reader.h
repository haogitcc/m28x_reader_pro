#ifndef __READER_H__
#define __READER_H__

#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

#include "app_sys_setting.h"
#include "mid_timer.h"
#include "mid_task.h"
#include "mid_telnet.h"
#include "mid_net.h"
#include "serial_m6e.h"
#include "server_m6e.h"
#include "m6e_init.h"
#include "gpio_init.h"

#define M28X 1
#define MYD 2

#define Board MYD
#if Board==MYD
#define DEVICE "/dev/ttymxc1"
#define DEVICE_NAME "tmr:///dev/ttymxc1"

#define GPO_28 44
#define GPO_29 43
#define GPI_30 42
#define GPI_31 41

//#define GPO1_DEVICE "/sys/class/gpio/gpio108/value"
#define GPO2_DEVICE "/sys/class/gpio/gpio44/value"
#define GPO3_DEVICE "/sys/class/gpio/gpio43/value"
#define GPO4_DEVICE "/sys/class/gpio/gpio42/value"
#define GPO5_DEVICE "/sys/class/gpio/gpio41/value"

#else //Board==M28X
#define DEVICE "/dev/ttySP0"
#define DEVICE_NAME "tmr:///dev/ttySP0"
#define GPO_28 60
#define GPO_29 61
#define GPI_30 61
#define GPI_31 63

//#define GPO1_DEVICE "/sys/class/gpio/gpio108/value"
#define GPO2_DEVICE "/sys/class/gpio/gpio60/value"
#define GPO3_DEVICE "/sys/class/gpio/gpio61/value"
#define GPO4_DEVICE "/sys/class/gpio/gpio62/value"
#define GPO5_DEVICE "/sys/class/gpio/gpio63/value"

#endif

#endif//__READER_H__

