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
#include "module_init.h"
#include "sqlite_m6e.h"
#include "client_data.h"

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
	int isRead = 0;
	interrupt();
  	sys_config_init();  
  	sys_config_load(0);
	mid_task_init();	
	mid_timer_init();

	mid_connect();
	tmr_file_init();
	telnetd_init(1);
	shell_play_init();
	pthread_tag_init();
	sysSettingSetInt("enable_cache", 1); //默认缓存
	mid_task_create_ex("update", (mid_func_t)m6e_start, NULL);
module:
	if(0 != module_init())
	{
		i++;
		if( 3 == i) {
			system("reboot");
		}
		goto module;
	} 
	if(0 == create_sqlite())
		sqlite_get_count();
	/*if(0 == socket_connect()) {
		//下载数据且解析数据
		get_server_reader();
		printf("64!\n");
		get_server_time();
	} */
	//socket_close();
	mid_task_create_ex("connect_server", (mid_func_t)client_start, NULL);
	sleep(6);
	printf("module_startReading\n");
	/*module_autoread_get(&isRead);
	if(isRead)
		mid_task_create_ex("startRead", (mid_func_t)module_startReading, NULL);
	else 
		module_send_set(0);
*/
	module_startReading();
	return 0;
}
