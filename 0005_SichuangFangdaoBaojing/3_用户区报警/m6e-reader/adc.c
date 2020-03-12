#include<stdio.h>	/* using printf()        */
#include<stdlib.h>      /* using sleep()         */
#include<fcntl.h>       /* using file operation  */
#include<sys/ioctl.h>   /* using ioctl()         */

static int fd;

int lradc_init()
{
	int time = 100;
	double val;
	
	fd = open("/dev/magic-adc", 0);
	if(fd < 0){
		printf("open error by APP- %d\n",fd);
		close(fd);
		return -1;
	}
	return 0;
}

int lradc_select()
{
  	struct timeval tv;
	int timeoutMs = 5;
	int ret = -1;
	int iRes;
	int val;
	
    tv.tv_sec = timeoutMs/1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;	
	while(1) {
		select(0,NULL,NULL,NULL,&tv);
		ioctl(fd, 26, &iRes);
		val = (iRes * 3.7)/4096.0;
		if(val > 3)
			gpio_status_set(1);
		else
			gpio_status_set(0);
	}
}

