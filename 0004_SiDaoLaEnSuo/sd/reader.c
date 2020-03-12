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
#include "module_client.h"
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
	mid_task_create_ex("update", (mid_func_t)m6e_start, NULL);

	printf("GPIO start!\n");
	gpio_init();
	if(gpio_create() < 0) {
		gpio_create();
	}

	//connect server
	if(0 !=socket_connect()) {
		gpio_write_60_v("1");
		//sleep(10);
		
	} else {
		gpio_write_60_v("0");
	}
	printf("module start!\n");

module:
	if(0 == module_init())
	{
		module_gpitriger();
	} else {
		i++;
		if( 3 == i) {
			gpio_write_60_v("1");
			system("reboot");
		}
		goto module;
	}
	gpio_write_60(3000*1000);
	printf("pthread_tag_init!\n");
	pthread_tag_init();
	mid_task_create_ex("client", (mid_func_t)client_start, NULL);
	//mid_task_create_ex("gpio", (mid_func_t)gpio_poll, NULL);
	printf("m6e_start!\n");
	gpio_poll();
	//m6e_start();
	return 0;
}
