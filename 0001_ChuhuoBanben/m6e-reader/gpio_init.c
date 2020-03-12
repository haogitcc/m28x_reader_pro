#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <poll.h>
#include "gpio_init.h"
#include "app_sys_setting.h"

#define MSG(args...) printf(args);

//#define GPO1_DEVICE "/sys/class/gpio/gpio108/value"
#define GPO2_DEVICE "/sys/class/gpio/gpio60/value"
#define GPO3_DEVICE "/sys/class/gpio/gpio61/value"
#define GPO4_DEVICE "/sys/class/gpio/gpio62/value"
#define GPO5_DEVICE "/sys/class/gpio/gpio63/value"

static int fdo1,fdo2,fdi;
static struct timeval tv;

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
	if(fd < 0 ){
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

int gpio_read(int pin)
{
	char value_str[3];
	char path[64];
	int fd;

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value",pin);
	fd = open(path, O_RDONLY);
	if(fd < 0 ){
		MSG("Failed to open gpio value for writing!\n");
		return -1;
	}

	if(read(fd, value_str, 3) < 0) {
		MSG("Failed to read value!\n");
		return -1;
	}

	close(fd);
	return atoi(value_str);
}

int gpio_edge(int pin, int edge)
{
	const char dir_str[] = "none\0rising\0falling\0both";

	int ptr;
	char path[64];
	int fd;

	switch(edge) {
		case 0:
			ptr = 0;
			break;
		case 1:
			ptr = 5;
			break;
		case 2:
			ptr = 12;
			break;
		case 3:
			ptr = 20;
			break;
		default:
			ptr = 0;
	}

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/edge", pin);
	fd = open(path, O_WRONLY);
	if(fd < 0) {
		MSG("Failed to open gpio edge for writing!\n");
		return -1;
	}

	if(write(fd, &dir_str[ptr], strlen(&dir_str[ptr])) < 0) {
		MSG("Failed to set edge!\n");
		return -1;
	}

	close(fd);
	return 0;
}

void sysUsecTime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
}

int compare_time()
{
	struct timeval tv2;
	gettimeofday(&tv2, NULL);
	printf("paper 1 : %ld , %ld\n", tv.tv_sec, tv.tv_usec/1000);
	printf("paper 2 : %ld , %ld\n", tv2.tv_sec, tv2.tv_usec/1000);
	int time;  
	if(tv2.tv_usec > tv.tv_usec) {
		time =  (tv2.tv_sec - tv.tv_sec) * 1000 + (tv2.tv_usec - tv.tv_usec)/1000; 
	} else {
		time = (tv2.tv_sec - tv.tv_sec) * 1000 - (tv.tv_usec - tv2.tv_usec)/1000; 
	}
	return time;
}

/*int gpio_poll()
{
	int gpio_fd, ret,count;
	struct pollfd fds[1];
	char buff[10];
	gpio_fd = open("/sys/class/gpio/gpio62/value", O_RDONLY);
	if(gpio_fd< 0) {
		MSG("Failed to open value!\n");
		return -1;
	}

	gettimeofday(&tv, NULL);

	fds[0].fd = gpio_fd;
	fds[0].events = POLLPRI;
	ret = read(gpio_fd, buff, 10);
	if(ret == -1)
		MSG("read!\n");

	while(1) {
		ret = poll(fds,1,0);
		if(ret == -1)
			MSG("poll!\n");
		if(fds[0].revents & POLLPRI) {
			int time = compare_time();
			if(time < 400)
				continue;
			gettimeofday(&tv, NULL);
			
			ret = lseek(gpio_fd, 0, SEEK_SET);
			if(ret == -1)
				MSG("lseek!\n");
			ret = read(gpio_fd, buff, 10);
			if(ret == -1)
				MSG("read!\n");
			//printf("buff = %d\n", atoi(buff));
			sysSettingGetInt("count", &count, 1);
			count++;
			sysSettingSetInt("count", count);
			sys_config_timer();
		}

		//usleep(50000);
	}
}*/

void setTimer(int seconds, int mseconds)
{
	struct timeval temp;
	temp.tv_sec = seconds;
	temp.tv_usec = mseconds;
	select(0,NULL,NULL,NULL,&temp);
	return;
}

int gpio_open(char *device)
{
	int fd;
	fd = open(device,O_RDWR);
	if(fd < 0) {
		printf("Open file %s failed!\n",device);
		return -1;
	}

	return fd;
}

int gpio_close()
{
	//close(fd1);
	close(fdo1);
	close(fdo2);
	fdo1 = -1;
	fdo2 = -1;
	return 0;
}

int gpio_create()
{
	fdo1 = gpio_open(GPO2_DEVICE);
	fdo2 = gpio_open(GPO3_DEVICE);
	if(fdo1 < 0 | fdo2 < 0) {
		gpio_close();
		printf("open gpio60 and gpio61 failed!\n");
		return -1;
	}
	printf("open gpio60 and gpio61 success!\n");
	return 0;
}

void gpio_close_v()
{
	gpio_close(fdo1);
	gpio_close(fdo2);
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

/*int gpio_read(int fd)
{
	int flags = -1;
	lseek(fd, 0, SEEK_SET);
	if(read(fd, &flags, 1) < 0)
		return -1;
	return flags;
}
*/

int gpio_write_60( int timeout)
{
	if(fdo1 < 0)
		return -1;
	printf("gpo2_write\n");
	int bytes_read = -1;

	lseek(fdo1, 0, SEEK_SET);
	bytes_read = write(fdo1, "1" ,1);
	if(bytes_read < 0) {
		goto Err;
	}
	setTimer(0,timeout);
	lseek(fdo1, 0, SEEK_SET);
	bytes_read = write(fdo1, "0" ,1);
	if(bytes_read< 0) {
		goto Err;
	}

	return 0;

Err:
	printf("gpo2 write failed!\n");
	gpio_close();
	gpio_create();
	return -1;
}

int gpio_write_60_v( char *str)
{
	if(fdo1 < 0)
		return -1;
	printf("gpio60 start\n");
	//printf("gpoio60_write : %s\n", str);
	int bytes_read = -1;

	lseek(fdo1, 0, SEEK_SET);
	bytes_read = write(fdo1, str ,1);
	if(bytes_read < 0) {
		goto Err;
	}
	printf("gpio60 write success!\n");
	return 0;
Err:
	printf("gpio60 write failed!\n");
	gpio_close();
	gpio_create();
	return -1;
}

int gpio_write_61_v(char * str)
{
	if(fdo2 < 0)
		return -1;
	//printf("gpoio61_write : %s\n", str);
	int bytes_read = -1;

	lseek(fdo2, 0, SEEK_SET);
	bytes_read = write(fdo2, str , 1);
	if(bytes_read < 0) {
		goto Err;
	}
	return 0;
Err:
	printf("gpio61 write failed!\n");
	gpio_close();
	gpio_create();
	return -1;
}

int gpio_write_61(int timeout)
{
	//printf("gpo2_write\n");
	int bytes_read = -1;

	lseek(fdo2, 0, SEEK_SET);
	bytes_read = write(fdo2, "0" ,1);
	if(bytes_read < 0) {
		goto Err;
	}
	setTimer(0,timeout);
	lseek(fdo2, 0, SEEK_SET);
	bytes_read = write(fdo2, "1" ,1);
	if(bytes_read< 0) {
		goto Err;
	}

	return 0;

Err:
	printf("gpo2 write failed!\n");
	//gpo_close();
	//gpo_init();
	return -1;
}


