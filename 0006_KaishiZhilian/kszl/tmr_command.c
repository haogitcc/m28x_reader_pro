#include <stdio.h>
#include <stdlib.h>

#include "tmr_command.h"
#include "tmr_file.h"
#include "cJSON.h"
#include "mid_timer.h"
#include "module_init.h"

static int request_first = 0;

typedef enum Status_Code {
	CODE_NORMAL = 0x01,                        //数据正常
	CODE_NO_ANTNENA = 0x02,                //读写器未接天线
	CODE_POWER_VAILED = 0x03,             //功率过大或过小
	CODE_ANTENNA_MISMATCH = 0x04,     //天线设置与读写器不匹配
} Status_Code;


//从str1中查找str2的个数
int findChildCnt(char* str1, char* str2)
{
	int len = strlen(str2);
	int cnt = 0;
	while(str1 = strstr(str1, str2))
	{
		cnt++;
		str1 += len;
	}

	return cnt;
}

char *returnJson(char * str)
{
	int len = strlen(str);
	//printf("returnJson : %d\n", len);
	char *buf = (char *)malloc(len+4);
	sprintf(buf, "<%s>", str);
	return buf;
}

void returnJson_v(char *str, char *value)
{
	//int len = strlen(str);
	int i = 1, j =0;
	//char *str1 = strchr(str, '<');
	//char *str2 = strchr(str, '>');
	while(str[i] != '>') {
		if(str[i] == '<') {
			i++;
		} else {	
			value[j] = str[i];
			i++;
			j++;
		}
	}
		
	//if(!str2)
	//	return NULL;
	//int len2 = strlen(str2);
	//strncpy(value, str, len -len2);
}

void parse_server_port(char *str)
{
	int ip1, ip2, ip3, ip4;
	int port;
	char ip[64];
	sscanf(str, "%d.%d.%d.%d:%d", &ip1,  &ip2, &ip3, &ip4, &port);
	printf("%d,%d,%d,%d", ip1,ip2,ip3,ip4 );
	sprintf(ip, "%d.%d.%d.%d" , ip1,ip2,ip3,ip4);
	sysSettingSetString("serverip", ip);
	sysSettingSetInt("serverport", port);
}

int parse_antenna(char *str)
{
	int ant1, ant1_power;
	int ant2, ant2_power;
	int ant3, ant3_power;
	int ant4, ant4_power;

	int ret = sscanf(str, "1,%d,%d;2,%d,%d;3,%d,%d;4,%d,%d", &ant1,&ant1_power, &ant2, &ant2_power, &ant3, &ant3_power,  &ant4, &ant4_power);
	if(ret <= 0) {
		printf("parse_antenna error!\n");
		return -1;
	}
	if(0 == get_ant_status())
		return CODE_NO_ANTNENA;
	ret = module_ant_judge(ant1, ant2, ant3, ant4);
	if(0 == ret)
		return CODE_ANTENNA_MISMATCH;
	sysSettingSetInt("ant1", ant1);
	sysSettingSetInt("ant2", ant2);
	sysSettingSetInt("ant3", ant3);
	sysSettingSetInt("ant4", ant4);
	
	if(ant1_power >=500 && ant1_power <= 3000)
		sysSettingSetInt("ant1_power", ant1_power);
	else
		return CODE_POWER_VAILED;
	if(ant2_power >=500 && ant2_power <= 3000)
		sysSettingSetInt("ant2_power", ant2_power);
	else
		return CODE_POWER_VAILED;
	if(ant3_power >=500 && ant3_power <= 3000)
		sysSettingSetInt("ant3_power", ant3_power);
	else
		return CODE_POWER_VAILED;
	if(ant4_power >=500 && ant4_power <= 3000)
		sysSettingSetInt("ant4_power", ant4_power);
	else
		return CODE_POWER_VAILED;

	return CODE_NORMAL;
}

int cache_check(int cache, int time)
{
	if(1 == cache) {
		printf("cache_check delete\n");
		mid_timer_delete((mid_timer_f)mid_tagdata_cache, 0);
	} else {
		socket_data_close();
		if(time < 2000)
			mid_timer_create(2 ,1, (mid_timer_f)mid_tagdata_cache, 0);
		else 
			mid_timer_create(time/1000 ,1, (mid_timer_f)mid_tagdata_cache, 0);
	}
	printf("cache_check\n");
}

int parse_command(char* vlaue)
{
	if(!vlaue)
		return -1;
	char str[1024];
	char str1[1024];
	memset(str, 0, 1024);
	memset(str1, 0, 1024);
	/*int ret = sscanf(vlaue, "<%s>", str);
	printf("parse json1: %s\n", str);
	if(ret <= 0) {
		printf("sscanf error\n");
	}*/
	returnJson_v(vlaue, str);
	if(!str)
		return -1;
	printf("parse json2: %s\n", str);
	cJSON *jsonroot = cJSON_Parse(str);
	if(!jsonroot) {
		printf("cjson error\n");
		return -1;
	}
	char* msg_id = cJSON_GetObjectItem(jsonroot,"msg_id")->valuestring;
	char* cmd = cJSON_GetObjectItem(jsonroot,"cmd")->valuestring;
	char *source = cJSON_GetObjectItem(jsonroot, "source")->valuestring;
	int flags = 1;
	printf("cmd : %s\n", cmd);
	if(!strcmp(cmd, "10001")) {
		//查询读写器状态
		get_reader_status(msg_id, cmd, source);
	} else if(!strcmp(cmd, "10002") ) {
		//设置读写器参数
		cJSON *taskArry = cJSON_GetObjectItem(jsonroot,"data");
		int time = cJSON_GetObjectItem(taskArry, "upload_time")->valueint;
		int enable_cache = cJSON_GetObjectItem(taskArry, "enable_cache")->valueint;
		int auto_read = cJSON_GetObjectItem(taskArry, "auto_start_read")->valueint;
		module_autoread_set(auto_read);
		char *server_addr = cJSON_GetObjectItem(taskArry, "server_addr")->valuestring;
		char *ant = cJSON_GetObjectItem(taskArry, "antenna")->valuestring;
		flags = parse_antenna(ant);
		printf("time : %d, enable: %d, server_addr: %s, ant: %s, flags: %d\n", time, enable_cache, server_addr, ant, flags);
		reponse_server(msg_id, cmd, flags, source);
		//if(1 == flags) {
			if(strlen(server_addr) > 10)
				parse_server_port(server_addr);
			sysSettingSetInt("time", time);
			int old_cache = 0;
			sysSettingGetInt("enable_cache", &old_cache, 1);
			sysSettingSetInt("enable_cache", enable_cache);
			sys_config_save();
			if(old_cache != enable_cache) {
				cache_check(old_cache,time);
				//sysSettingSetInt("enable_cache", enable_cache);
				/*if(1 == old_cache) {
					mid_timer_delete((mid_timer_f)mid_tagdata_cache, 0);
				} else {
					socket_data_close();
					if(time < 2000)
						mid_timer_create(2 ,1, (mid_timer_f)mid_tagdata_cache, 0);
					else 
						mid_timer_create(time/1000 ,1, (mid_timer_f)mid_tagdata_cache, 0);
				}
				*/
				/*mid_timer_delete((mid_timer_f)mid_tagdata_cache, 0);
				if(old_cache)
					mid_tagdata_send_v(1);
				else
					mid_tagdata_send_v(2);*/
			//}
		}
	} else if(!strcmp(cmd, "10003")) {
		//cJSON *taskArry = cJSON_GetObjectItem(jsonroot,"data");
		//重启读写器
		reponse_server(msg_id, cmd, flags, source);
		module_flags_set(0);
		sleep(2); //等待读卡停止
		system("reboot");
	} else if( !strcmp(cmd, "10004")) {
		cJSON *taskArry = cJSON_GetObjectItem(jsonroot,"data");
		int time = cJSON_GetObjectItem(taskArry, "upload_time")->valueint;
		int enable_cache = cJSON_GetObjectItem(taskArry, "enable_cache")->valueint;
		int auto_read = cJSON_GetObjectItem(taskArry, "auto_start_read")->valueint;
		module_autoread_set(auto_read);
		char *server_addr = cJSON_GetObjectItem(taskArry, "server_addr")->valuestring;
		char *ant = cJSON_GetObjectItem(taskArry, "antenna")->valuestring;
		flags = parse_antenna(ant);
		if(1 == flags) {
			int old_cache = 0;
			sysSettingGetInt("enable_cache", &old_cache, 1);
			parse_server_port(server_addr);
			sysSettingSetInt("time", time);
			sysSettingSetInt("enable_cache", enable_cache);
			sys_config_save();
			if(request_first) {
				if(old_cache != enable_cache) {
					cache_check(old_cache,time);
				}
			}
			if(0 == request_first)
				request_first = 1;
		}
	}else if(!strcmp(cmd, "10005")) {
		cJSON *taskArry = cJSON_GetObjectItem(jsonroot,"data");
		int result = cJSON_GetObjectItem(taskArry, "result")->valueint;
		cJSON_Delete(jsonroot);
		return result;
	} else if(!strcmp(cmd, "10006")) {
		//cJSON *taskArry = cJSON_GetObjectItem(jsonroot,"data");
		//开始读卡
		module_send_set(1);
		reponse_server(msg_id, cmd, flags, source);

	} else if(!strcmp(cmd, "10007")) {
		//cJSON *taskArry = cJSON_GetObjectItem(jsonroot,"data");
		//停止读卡
		module_send_set(0);
		reponse_server(msg_id, cmd, flags, source);
	} else if(!strcmp(cmd, "10008")) {
		cJSON *taskArry = cJSON_GetObjectItem(jsonroot,"data");
		char *server_time = cJSON_GetObjectItem(taskArry, "server_time")->valuestring;	
		if(server_time)
			SetSystemTime(server_time);
	} else {
		cJSON_Delete(jsonroot);
		return -1;
	}
	cJSON_Delete(jsonroot);
}

void get_reader_status(char *msg_id, char* cmd, char *source)
{
	cJSON* root = cJSON_CreateObject();
	cJSON* img = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "msg_id", cJSON_CreateString(msg_id));
	cJSON_AddItemToObject(root, "cmd", cJSON_CreateString(cmd));
	cJSON_AddItemToObject(root, "source", cJSON_CreateString(source));
	cJSON_AddItemToObject(root, "data", img);
	
	char device_id[64];
	sysSettingGetString("id", device_id, 64, 1);
	cJSON_AddStringToObject(img, "device_id", device_id);

	int upload_time;
	sysSettingGetInt("time", &upload_time, 1);
	cJSON_AddNumberToObject(img , "upload_time",  upload_time);

	char ip[64];
	int port;
	sysSettingGetString("serverip", ip, 64, 1);
	sysSettingGetInt("serverport",  &port, 1);
	char str[128];
	sprintf(str, "%s:%d", ip, port);
	cJSON_AddStringToObject(img, "server_addr", str);

	int isRead = 0;
	module_send_get(&isRead);
	cJSON_AddNumberToObject(img , "is_reading_tag",  isRead);

	int isAuto = 0;
	module_autoread_get(&isAuto);
	cJSON_AddNumberToObject(img , "auto_start_read",  isAuto);
	int enable_cache = 0;
	sysSettingGetInt("enable_cache", &enable_cache, 1);
	cJSON_AddNumberToObject(img , "enable_cache",  enable_cache);

	
	int ant1, ant2, ant3, ant4;
	int ant1_power, ant2_power, ant3_power, ant4_power;
	
	sysSettingGetInt("ant1", &ant1, 1);
	sysSettingGetInt("ant2", &ant2, 1);
	sysSettingGetInt("ant3", &ant3, 1);
	sysSettingGetInt("ant4", &ant4, 1);
	sysSettingGetInt("ant1_power", &ant1_power, 1);
	sysSettingGetInt("ant2_power", &ant2_power, 1);
	sysSettingGetInt("ant3_power", &ant3_power, 1);
	sysSettingGetInt("ant4_power", &ant4_power, 1);
	
	char antenna[128];
	sprintf(antenna, "1,%d,%d;2,%d,%d;3,%d,%d;4,%d,%d", ant1, ant1_power, ant2, ant2_power, ant3, ant3_power, ant4, ant4_power);
	cJSON_AddStringToObject(img, "antenna", antenna);
	int status = 0;
	if(0 == get_ant_status())
		status = 2;
	else
		status = 1;
	cJSON_AddNumberToObject(img, "is_ok", status);

	//printf(cJSON_Print(root));
	char *sp = cJSON_Print(root);
	char *value = returnJson(sp);
	int len = strlen(value);
	socket_send(value, len);
	free(value);
	free(sp);
	cJSON_Delete(root);
}


void reponse_server(char *msg_id, char* cmd, int flags, char *source)
{
	cJSON* root = cJSON_CreateObject();
	cJSON* img = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "msg_id", cJSON_CreateString(msg_id));
	cJSON_AddItemToObject(root, "cmd", cJSON_CreateString(cmd));
	cJSON_AddItemToObject(root, "source", cJSON_CreateString(source));
	cJSON_AddItemToObject(root, "data", img);
	char device_id[64];
	sysSettingGetString("id", device_id, 64, 1);
	cJSON_AddStringToObject(img, "device_id", device_id);
	cJSON_AddNumberToObject(img, "result", flags);
	char *sp = cJSON_Print(root);
	char *str = returnJson(sp);
	int len = strlen(str);
	socket_send(str, len);
	free(str);
	free(sp);
	cJSON_Delete(root);
}

void get_server_reader()
{
	cJSON* root = cJSON_CreateObject();
	cJSON* img = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "msg_id", cJSON_CreateString("KL180002"));
	cJSON_AddItemToObject(root, "cmd", cJSON_CreateString("10004"));	
	cJSON_AddItemToObject(root, "source", cJSON_CreateString("client"));
	cJSON_AddItemToObject(root, "data", img);	
	char device_id[64];
	sysSettingGetString("id", device_id, 64, 1);
	cJSON_AddStringToObject(img, "device_id", device_id);
	char *sp = cJSON_Print(root);
	char *str = returnJson(sp);
	int len = strlen(str);
	socket_send(str, len);
	//socket_receive(2000);
	free(str);
	free(sp);
	cJSON_Delete(root);
}

void get_server_time()
{
	cJSON* root = cJSON_CreateObject();
	cJSON* img = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "msg_id", cJSON_CreateString("KL180002"));
	cJSON_AddItemToObject(root, "cmd", cJSON_CreateString("10008"));	
	cJSON_AddItemToObject(root, "source", cJSON_CreateString("client"));
	cJSON_AddItemToObject(root, "data", img);	
	char device_id[64];
	sysSettingGetString("id", device_id, 64, 1);
	cJSON_AddStringToObject(img, "device_id", device_id);

	int upload_time;
	sysSettingGetInt("time", &upload_time, 1);
	cJSON_AddNumberToObject(img , "upload_time",  upload_time);
	
	char ip[64];
	int port;
	sysSettingGetString("serverip", ip, 64, 1);
	sysSettingGetInt("serverport",  &port, 1);
	char str[128];
	sprintf(str, "%s:%d", ip, port);
	cJSON_AddStringToObject(img, "server_addr", str);

	int isRead = 0;
	module_send_get(&isRead);
	cJSON_AddNumberToObject(img , "is_reading_tag",  isRead);

	int isAuto = 0;
	module_autoread_get(&isAuto);
	cJSON_AddNumberToObject(img , "auto_start_read",  isAuto);
	int enable_cache = 0;
	sysSettingGetInt("enable_cache", &enable_cache, 1);
	cJSON_AddNumberToObject(img , "enable_cache",  enable_cache);

	
	int ant1, ant2, ant3, ant4;
	int ant1_power, ant2_power, ant3_power, ant4_power;
	
	sysSettingGetInt("ant1", &ant1, 1);
	sysSettingGetInt("ant2", &ant2, 1);
	sysSettingGetInt("ant3", &ant3, 1);
	sysSettingGetInt("ant4", &ant4, 1);
	sysSettingGetInt("ant1_power", &ant1_power, 1);
	sysSettingGetInt("ant2_power", &ant2_power, 1);
	sysSettingGetInt("ant3_power", &ant3_power, 1);
	sysSettingGetInt("ant4_power", &ant4_power, 1);
	
	char antenna[128];
	sprintf(antenna, "1,%d,%d;2,%d,%d;3,%d,%d;4,%d,%d", ant1, ant1_power, ant2, ant2_power, ant3, ant3_power, ant4, ant4_power);
	cJSON_AddStringToObject(img, "antenna", antenna);
	int status = 0;
	if(0 == get_ant_status())
		status = 2;
	else
		status = 1;
	cJSON_AddNumberToObject(img, "is_ok", status);
	
	char *sp = cJSON_Print(root);
	char *str1 = returnJson(sp);
	int len = strlen(str1);
	socket_send(str1, len);
	free(str1);
	free(sp);
	//socket_receive(2000);
	mid_timer_create(60, 1, (mid_timer_f)get_server_time, NULL);
	cJSON_Delete(root);
}

int mid_tagdata_server(char* str, int number, int fd)
{
	int ret;
	cJSON* root = cJSON_CreateObject();
	cJSON* img = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "msg_id", cJSON_CreateString("KL180002"));
	cJSON_AddItemToObject(root, "cmd", cJSON_CreateString("10005"));
	cJSON_AddItemToObject(root, "source", cJSON_CreateString("client"));
	cJSON_AddItemToObject(root, "data", img);	
	char device_id[64];
	sysSettingGetString("id", device_id, 64, 1);
	cJSON_AddStringToObject(img, "device_id", device_id);

	cJSON_AddNumberToObject(img, "count", number);
	if(0 == number)
		cJSON_AddStringToObject(img, "epc", "");
	else
		cJSON_AddStringToObject(img, "epc", str);
	char *sp = cJSON_Print(root);

	char *value = returnJson(sp);
	//printf("mid_tagdata_server: %s\n", value);
	int len = strlen(value);
	ret = socket_send_ex(value, len, fd);
	free(value);
	free(sp);
	cJSON_Delete(root);
	return ret;
}



