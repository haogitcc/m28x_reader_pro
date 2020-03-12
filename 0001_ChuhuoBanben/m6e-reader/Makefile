#CC=/opt/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-fsl-linux-gnueabi-gcc

CC=/opt/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/bin/arm-fsl-linux-gnueabi-gcc
#CC=gcc

reader:reader.o app_sys_setting.o ind_cfg.o ind_string.o mid_msgq.o mid_mutex.o mid_net.o mid_select.o mid_task.o mid_telnet.o mid_timer.o ringbuf.o serialM6E.o server_m6e.o shell_play.o  telnetd_port.o osex_net.o m6e_upgrade.o gpio_init.o
	$(CC) -o reader reader.o app_sys_setting.o ind_cfg.o ind_string.o mid_msgq.o mid_mutex.o mid_net.o mid_select.o mid_task.o mid_telnet.o mid_timer.o ringbuf.o serialM6E.o server_m6e.o shell_play.o  telnetd_port.o osex_net.o m6e_upgrade.o gpio_init.o -lpthread  -I/opt/include -L/opt/lib/   -L./lib/libdhcpcd -lrt ./lib/libMecuryAPI.a


reader.o:reader.c app_sys_setting.h mid_timer.h mid_task.h mid_telnet.h mid_net.h serial_m6e.h m6e_init.h
	$(CC) -c  reader.c
app_sys_setting.o:app_sys_setting.c mid_mutex.h ind_cfg.h ind_string.h mid_timer.h
	$(CC) -c app_sys_setting.c
ind_cfg.o:ind_cfg.c ind_cfg.h ind_string.h
	$(CC) -c ind_cfg.c
ind_string.o:ind_string.c ind_string.h
	$(CC) -c ind_string.c
mid_msgq.o:mid_msgq.c mid_msgq.h mid_mutex.h
	$(CC) -c mid_msgq.c
mid_mutex.o:mid_mutex.c mid_mutex.h
	$(CC) -c mid_mutex.c
mid_net.o:mid_net.c  osex_net.h mid_net.h dhcpcd.h
	$(CC) -c mid_net.c  -L./lib/ -ldhcpcd
osex_net.o:osex_net.c osex_net.h
	$(CC) -c osex_net.c
mid_select.o:mid_select.c mid_select.h net_include.h mid_mutex.h
	$(CC) -c mid_select.c
mid_task.o:mid_task.c mid_task.h mid_mutex.h
	$(CC) -c mid_task.c
mid_telnet.o:mid_telnet.c mid_telnet.h telnetd_port.h mid_msgq.h mid_mutex.h ind_string.h
	$(CC) -c mid_telnet.c 
mid_timer.o:mid_timer.c mid_timer.h net_include.h mid_mutex.h mid_msgq.h mid_task.h
	$(CC) -c mid_timer.c -lrt
ringbuf.o:ringbuf.c ringbuf.h mid_mutex.h
	$(CC) -c ringbuf.c
serialM6E.o:serialM6E.c  serial_m6e.h
	$(CC) -c serialM6E.c 
server_m6e.o:server_m6e.c server_m6e.h mid_mutex.h tmr_utils.h tmr_params.h m6e_upgrade.h
	$(CC) -c server_m6e.c -lpthread
shell_play.o:shell_play.c shell.h app_sys_setting.h
	$(CC) -c shell_play.c
#sqlite_m6e.o:sqlite_m6e.c sqlite_m6e.h
#	$(CC) -c sqlite_m6e.c -lsqlite3
telnetd_port.o:telnetd_port.c telnetd_port.h mid_task.h
	$(CC) -c telnetd_port.c
m6e_upgrade.o:m6e_upgrade.c m6e_upgrade.h
	$(CC) -c m6e_upgrade.c
gpio_init.o:gpio_init.c gpio_init.h
	$(CC) -c gpio_init.c

clean:
	rm reader  *.o
