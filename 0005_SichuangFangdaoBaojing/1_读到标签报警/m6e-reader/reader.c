#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

#include "mid_timer.h"
#include "mid_task.h"
#include "mid_telnet.h"
#include "mid_net.h"
#include "serial_m6e.h"
#include "server_m6e.h"
#include "gpio_init.h"
#include "module_init.h"

void handle_pipe(int sig)
{

}
void *interrupt()
{
  	struct sigaction action;
  	action.sa_handler = handle_pipe;
  	sigemptyset(&action.sa_mask);
  	action.sa_flags = 0;
  	sigaction(SIGPIPE, &action, NULL);
	return NULL;
}

int main(int argc, char **argv)
{
	int ret = -1;
	int i = 0;
	interrupt();
  	sys_config_init();  
  	sys_config_load(0);
	mid_task_init();	
	mid_timer_init();

	mid_connect();

	telnetd_init(1);
	shell_play_init();

	printf("GPIO start!\n");
	gpio_init();
	printf("module start!\n");

start:
	if(0 != module_init()) { //±‹√‚≥ı ºªØƒ£øÈ ß∞‹
		i++;
		if(i == 3) {
			system("reboot");
		}
		sleep(1);
		goto start;
	} else {
		//module_gpitriger();
		mid_task_create_ex("module",( mid_func_t) module_startReading, NULL);
	}

	printf("pthread_tag_init!\n");
	pthread_tag_init();
	m6e_start();
	return 0;
}
