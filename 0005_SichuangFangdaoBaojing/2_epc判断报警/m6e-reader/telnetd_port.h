
#ifndef __TELNETD_PORT_H__
#define __TELNETD_PORT_H__

#include "mid_task.h"

void telnetd_link_state_display_task_create(mid_func_t entry);
void telnetd_port_task_create(mid_func_t entry, void *arg);

/*
	核对账号

	返回值：
		0 无败
		1 有效
 */
int telnetd_port_user_ok(char *user);

/*
	核对密码

	返回值：
		0 无败
		1 有效
 */
int telnetd_port_pass_ok(char *pass);

enum {
	eTelnet_state_idle = 0,	//空闲
	eTelnet_state_connect,	//连接
	eTelnet_state_session,	//登陆成功，正式会话
} eTelnet_state;
/*
	telnet状态改变时会调用该值
	state 取值范围为eTelnet_state
 */
void telnetd_port_post_state(int state);

#endif//__TELNETD_PORT_H__

