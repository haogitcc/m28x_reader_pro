#include<stdio.h>
#include<stdint.h>


void module_to_machine_begin(char *message,  int stu)
{
	int id = -1;
	sysSettingGetInt("id", &id, 1);
	char epc[64];
	sysSettingGetString("epc", epc, 64, 1);
	sprintf(message, "{\"id\":%d,\"epc\":\"%s\",\"status\":%d}", id, epc, stu);
}

int module_machine_begin_response(char *str)
{
	int operation,time,state= -1,ret;
	printf("module_machine_begin_response: %s\n", str);
	ret = sscanf(str, "{\"operation\":\"%d\",\"time\":\"%d\",\"state\":\"%d\"}",&operation, &time, &state);
	printf("operation : %d, time: %d ,state : %d\n", operation, time, state);
	if(state != 0  || ret <=0){
		//sysSettingSetString("epc", "0000");
		//sysSettingSetInt("count", 0);
		//sys_config_save();
		//app_module_clear();
		return state;
	}
	sysSettingSetInt("operation",operation );
	sysSettingSetInt("time", time);
	sys_config_save();
	return state;
}

int module_to_machine_end(char *message)
{
	int operation  = -1, ret = -1, serial = -1, number = 0;
	int count1 = 0, count2 = 0;
	sysSettingGetInt("operation", &operation, 1);
	sysSettingGetInt("serial", &serial, 1);
	sysSettingGetInt("count1", &count1, 1);
	sysSettingGetInt("count2", &count2, 1);
	if(count1 >= count2)
		number = count1-count2;
	else
		number = count2-count1;
	ret = sprintf(message, "{\"operation\":%d,\"serial\":%d,\"number\":%d}",operation, serial,number);
	if(ret < 0)
		return -1;
	return 0;
}

int module_to_machine_end_response(char *str)
{
	int state = -1, ret = -1;
	printf("module_to_machine_end_response : %s\n", str);
	ret = sscanf(str, "{\"state\":\"%d\"}", &state);
	if(ret <= 0)
		return -1;
	return state;
}

void app_module_clear()
{
	//sysSettingSetString("epc", "0000");
	sysSettingSetInt("flags", 0);
	sysSettingSetInt("count1", 0);
	sysSettingSetInt("count2", 0);
	sysSettingSetInt("operation", 0);
	sys_config_save();	
}

int module_parse(char *str)
{
	int value = module_flags_get();
	int count1;
	int state = 0;
	switch(value) {
		case 1:
			state = module_machine_begin_response(str);
			break;
		case 2:
			state = module_to_machine_end_response(str);
			break;
		default:
			return;
	}
	printf("module_parse : %d, %d\n",value,  state);
	if((0 == state || 2 == state || 3 == state) && (2 == value)) {
		app_module_clear();
		gpio_write_60(3000 * 1000);
	}else if((1 == value) &&(2 == state || 1 == state)) {
		gpio_write_61_v("0");
	}else if((1 == value) &&(0 == state)) {
		//gpio_write_61_v("1");
		gpio_write_60(1000 *1000);
		sleep(1);
		gpio_write_60(1000 *1000);
		//gpio_write_60(1000 * 1000);
	} 

	sysSettingGetInt("count1", &count1, 1);
	if((count1 >0) && (0 == state))
		socket_send_end();
	return state;
}

