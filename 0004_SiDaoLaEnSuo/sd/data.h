#ifndef __DATA_H__
#define __DATA_H__


void module_to_machine_begin(char *message,  int stu)
int module_machine_begin_response(char *str);
int module_to_machine_end(char *messagel);
int module_to_machine_end_response(char *str);
int module_parse(char *str);
void app_module_clear();

#endif//__NET_INCLUDE_H__

