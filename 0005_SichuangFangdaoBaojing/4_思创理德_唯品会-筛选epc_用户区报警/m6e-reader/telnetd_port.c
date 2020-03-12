

#include "telnetd_port.h"
#include "mid_task.h"

void telnetd_port_task_create(mid_func_t entry, void *arg)
{
	//	app_telnetd_task_create(entry, arg);
	//mid_telnetd_task_create(entry, arg);
	mid_task_create("telneted",entry,arg);

}

int telnetd_port_user_ok(char *user)
{
    if (user && strcmp(user, "fuwit") == 0)
        return 1;
    else if (user && strcmp(user, "user") == 0)
        return 1;
    return 0;
}

int telnetd_port_pass_ok(char *pass)
{
    if (pass && strcmp(pass, "123456") == 0)
        return 1;
    else if (pass && strcmp(pass, "28780808") == 0)
        return 1;
    return 0;
}



