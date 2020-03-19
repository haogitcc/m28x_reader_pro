#include "reader.h"

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
	interrupt();
  	sys_config_init();  
  	sys_config_load(0);
	mid_task_init();	
	mid_timer_init();

	mid_connect();

	telnetd_init(1);
	shell_play_init();

	gpio_init();

	if(0 == m6e_init(DEVICE_NAME))
	{
		m6e_configuration_init();
		m6e_destory();
	}
	if(serial_open(DEVICE) < 0)
		return -1;
	else {
		m6e_baudrate(460800);
		serial_flush();
	}
	pthread_tag_init();
	m6e_start();
	return 0;
}
