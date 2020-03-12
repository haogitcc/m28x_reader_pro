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
	gpio_init();
	pthread_tag_init();

	printf("GPIO start!\n");
	if(gpo_init() < 0) {
		gpo_init();
	}
	
Start_init_module:
	printf("module start!\n");
	if(0 != module_init())
	{
		printf("module gpi start1!\n");
		i++;
		if(3 == i)
			system("reboot");
		goto Start_init_module;
	}

	mid_task_create_ex("gpio_poll", (mid_func_t)gpio_poll, NULL);
	mid_task_create_ex("check_status", (mid_func_t)check_status, NULL);
	
	printf("m6e_start!\n");
	m6e_start();
	return 0;
}
