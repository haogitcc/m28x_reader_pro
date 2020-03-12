#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>

#include "gpio_init.h"


//#define GPO1_DEVICE "/sys/class/gpio/gpio108/value"
#define GPO2_DEVICE "/sys/class/gpio/gpio60/value"
#define GPO3_DEVICE "/sys/class/gpio/gpio61/value"
#define GPO4_DEVICE "/sys/class/gpio/gpio62/value"
#define GPO5_DEVICE "/sys/class/gpio/gpio63/value"

#define MSG(args...) printf(args);


static int fd1,fd2;

void gpio_init()
{
	gpio_export(60);
	gpio_direction(60, 1);
	gpio_write(60, 0);

	gpio_export(61);
	gpio_direction(61, 1);
	gpio_write(61, 1);

	gpio_export(62);
	gpio_direction(62, 0);
	//gpio_edge(62, 1);
	
	gpio_export(63);
	gpio_direction(63, 0);
	//gpio_edge(62, 1);
	
}

int gpio_export(int pin)
{
	char buffer[64];
	int len;
	int fd;

	fd = open("/sys/class/gpio/export" ,O_WRONLY);
	if(fd < 0) {
		MSG("Failed to open export for writing!\n");
		return -1;
	}

	len = snprintf(buffer, sizeof(buffer), "%d", pin);
	if(write(fd, buffer, len) < 0) {
		MSG("Failed to export gpio!\n");
	}

	close(fd);
	return 0;
}

int gpio_unexport(int pin)
{
	char buffer[64];
	int len;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if(fd < 0) {
		MSG("Failed to open unexport for writing!\n");
		return -1;
	}

	len = snprintf(buffer, sizeof(buffer), "%d", pin);
	if(write(fd, buffer, len) < 0) {
		MSG("Failed to unexport gpio!\n");
	}

	close(fd);
	return 0;
}

int gpio_direction(int pin, int dir)
{
	const char dir_str[] = "in\0out";
	char path[64];
	int fd;

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction",pin);
	fd = open(path, O_WRONLY);
	if(fd < 0 ){
		MSG("Failed to open gpio direction for writing!\n");
		return -1;
	}

	if(write(fd, &dir_str[dir == 0? 0 : 3], dir == 0? 2 :3) < 0) {
		MSG("Failed to set direction!\n");
		return -1;
	}

	close(fd);
	return 0;
}

int gpio_write(int pin, int value)
{
	const char dir_str[] = "01";
	char path[64];
	int fd;

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value",pin);
	fd = open(path, O_WRONLY);
	if(fd < 0 ) {
		MSG("Failed to open gpio value for writing!\n");
		return -1;
	}

	if(write(fd, &dir_str[value == 0? 0 : 1], 1) < 0) {
		MSG("Failed to write value!\n");
		return -1;
	}

	close(fd);
	return 0;
}

void setTimer(int seconds, int mseconds)
{
	struct timeval temp;
	temp.tv_sec = seconds;
	temp.tv_usec = mseconds;
	select(0,NULL,NULL,NULL,&temp);
	return;
}

int gpo_open(char *device)
{
	int fd;
	fd = open(device,O_RDWR);
	if(fd < 0) {
		printf("Open file %s failed!\n",device);
		return -1;
	}

	return fd;
}

int gpo_close()
{
	//close(fd1);
	close(fd2);
	return 0;
}

int gpo_init()
{
/*	if((fd1 = gpo_open(GPO1_DEVICE)) < 0) {
		gpo_close();
		return -1;
	}
*/
	if((fd2 = gpo_open(GPO2_DEVICE)) < 0) {
		gpo_close();
		return -1;
	}

	return 0;
}


/*int gpo1_write()
{
	int bytes_read = -1;

	lseek(fd1, 0, SEEK_SET);
	bytes_read = write(fd1, "1" ,1);
	if(bytes_read < 0) {
		goto Err;
	}
	setTimer(0,20000-100);
	lseek(fd1, 0, SEEK_SET);
	bytes_read = write(fd1, "0" ,1);
	if(bytes_read< 0) {
		goto Err;
	}

	return 0;

Err:
	gpo_close();
	gpo_init();
	return -1;
}
*/
int gpo2_write()
{
	printf("gpo2_write\n");
	int bytes_read = -1;

	lseek(fd2, 0, SEEK_SET);
	bytes_read = write(fd2, "1" ,1);
	if(bytes_read < 0) {
		goto Err;
	}
	setTimer(0,10000 - 80);
	lseek(fd2, 0, SEEK_SET);
	bytes_read = write(fd2, "0" ,1);
	if(bytes_read< 0) {
		goto Err;
	}

	return 0;

Err:
	printf("gpo2 write failed!\n");
	gpo_close();
	gpo_init();
	return -1;
}



