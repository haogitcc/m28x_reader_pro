#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/poll.h> 

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
	gpio_edge(62, 2);
	
	gpio_export(63);
	gpio_direction(63, 0);
	gpio_edge(62, 2);
	
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

int gpio_write_v(int fd, int value)
{
	const char dir_str[] = "01";
	if(write(fd, &dir_str[value == 0? 0 : 1], 1) < 0) {
		MSG("Failed to write value!\n");
		return -1;
	}

	//close(fd);	
}

void setTimer(int seconds, int mseconds)
{
	struct timeval temp;
	temp.tv_sec = seconds;
	temp.tv_usec = mseconds;
	select(0,NULL,NULL,NULL,&temp);
	return;
}

int gpo60_set(int time, int fd)
{
	printf("gpo60_set\n");
	int bytes_read = -1;

	lseek(fd, 0, SEEK_SET);
	bytes_read = write(fd, "1" ,1);
	if(bytes_read < 0) {
		goto Err;
	}
	setTimer(0,time - 80);
	lseek(fd, 0, SEEK_SET);
	bytes_read = write(fd, "0" ,1);
	if(bytes_read< 0) {
		goto Err;
	}

	return 0;

Err:
	printf("gpo2 write failed!\n");
	return -1;
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
		printf("gpo_open 60 failed!\n");
		gpo_close();
		return -1;
	}

	return 0;
}


int gpio_poll()
{
	int gpi62_fd, ret, gpo60_fd, gpo61_fd;
	struct pollfd fds[1];
	char buff[10];
	int count = 0;
start:
	gpi62_fd = open("/sys/class/gpio/gpio62/value", O_RDONLY);
	if(gpi62_fd< 0) {
		close(gpi62_fd);
		MSG("Failed to open value!\n");
		goto start;
	}

	/*gpo60_fd = open("/sys/class/gpio/gpio60/value", O_RDWR);
	if(gpo60_fd< 0) {
		close(gpo60_fd);
		close(gpi62_fd);
		MSG("Failed to open value!\n");
		goto start;
	}*/
	
	gpo61_fd = open("/sys/class/gpio/gpio61/value", O_RDWR);
	if(gpo61_fd< 0) {
		close(gpo61_fd);
		//close(gpo60_fd);
		close(gpi62_fd);
		MSG("Failed to open value!\n");
		goto start;
	}

	/*gpi63_fd = open("/sys/class/gpio/gpio63/value", O_RDONLY);
	if(gpi63_fd< 0) {
		MSG("Failed to open value!\n");
		close(gpi62_fd);
		close(gpi63_fd);
		goto start;
	}*/

	//gettimeofday(&tv, NULL);

	fds[0].fd = gpi62_fd;
	fds[0].events = POLLPRI;
	ret = read(gpi62_fd, buff, 10);
	if(ret == -1)
		MSG("read!\n");

	/*fds[1].fd = gpi63_fd;
	fds[1].events = POLLPRI;
	ret = read(gpi63_fd, buff, 10);
	if(ret == -1)
		MSG("read!\n");
*/
	while(1) {
		//printf("gpio_poll 11111111111\n");
		ret = poll(fds,1,0);
		if(ret < 0) {
			MSG("poll!\n");
			continue;
		}
		//printf("gpio_poll 222222222222\n");
		if(fds[0].revents & POLLPRI) {
			ret = lseek(gpi62_fd, 0, SEEK_SET);
			if(ret == -1)
				MSG("lseek!\n");
			memset(buff, 0 , 10);
			ret = read(gpi62_fd, buff, 10);
			if(ret == -1)
				MSG("read!\n");
			//printf("buff : %s\n", buff);
			if(!strncmp(buff, "0", 1)) {
				struct timeval tv2;
				gettimeofday(&tv2, NULL);
				printf("begin paper 1 : %ld , %ld\n", tv2.tv_sec, tv2.tv_usec/1000);
				gpio_write_v(gpo61_fd, 0);
				//printf("gpio_poll 333333333\n");
				count = read_tag();
				//printf("gpio_poll 444444444444\n");
				struct timeval tv;
				gettimeofday(&tv, NULL);
				printf("end paper 1 : %ld , %ld\n", tv.tv_sec, tv.tv_usec/1000);
				gpio_write_v(gpo61_fd, 1);
			}
			//printf("gpio_poll 55555555555\n");
		}/* else if(fds[1].revents & POLLPRI){
			ret = lseek(gpi63_fd, 0, SEEK_SET);
			if(ret == -1)
				MSG("lseek!\n");
			ret = read(gpi63_fd, buff, 10);
			if(ret == -1)
				MSG("read!\n");
			//printf("buff = %d\n", atoi(buff));
			sysSettingGetInt("count2", &count2, 1);
			count2++;
			sysSettingSetInt("count2", count2);
			sys_config_timer();
		}
*/
		

		//usleep(50000);
	}
}

/*void setTimer(int seconds, int mseconds)
{
	struct timeval temp;
	temp.tv_sec = seconds;
	temp.tv_usec = mseconds;
	select(0,NULL,NULL,NULL,&temp);
	return;
}
*/
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



