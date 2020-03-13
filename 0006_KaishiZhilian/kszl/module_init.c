#include "tm_reader.h"
#include "serial_reader_imp.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <linux/tcp.h>
#include <stdint.h>
#include <fcntl.h>
#include "gpio_init.h"
#include "mid_timer.h"
#include "db_hash.h"
#include "mid_mutex.h"
#include "sqlite_m6e.h"

TMR_Reader r, *rp;
TMR_Region region = TMR_REGION_NA;
TMR_SR_UserConfigOp config;
TMR_GPITriggerRead triggerRead;
TMR_ReadPlan plan;
uint8_t *antennaList = NULL;
uint8_t antennaCount = 0;
int socket_data = -1;
static int antlen = 0;
TMR_TagOp op;
TMR_uint8List gpiPort;
uint8_t data[10] = {0};
TMR_SR_UserConfigOp config;
uint32_t db_size = 10;
TMR_ReadListenerBlock rlb;
TMR_ReadExceptionListenerBlock reb;

int isant1 = 0, isant2 = 0, isant3 = 0, isant4 = 0;
static int uniqueCount;
static mid_mutex_t g_mutex = NULL;

#define DEVICE_NAME "tmr:///dev/ttySP0"
#define numberof(x) (sizeof((x))/sizeof((x)[0]))

void callback(TMR_Reader *reader, const TMR_TagReadData *t, void *cookie);
void exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie);
void module_detroy();
void mid_tagdata_send(int flags);

void checkerr(TMR_Reader* rp, TMR_Status ret, int exitval, const char *msg)
{
	if (TMR_SUCCESS != ret)
	{
		//errx(exitval, "Error %s: %s\n", msg, TMR_strerr(rp, ret));
		printf("Error %s: %s\n",msg, TMR_strerr(rp, ret));
	}
}

int get_ant_status()
{
	return antlen;
}

int module_ant_flags(int port)
{
	printf("port : %d\n",port);
	switch(port) {
		case 1:
			isant1= 1;
			sysSettingSetInt("ant1", 1);
			break;
		case 2:
			isant2 = 1;
			sysSettingSetInt("ant2", 1);
			break;
		case 3:
			isant3 = 1;
			sysSettingSetInt("ant3", 1);
			break;
		case 4:
			isant4 = 1;
			sysSettingSetInt("ant4", 1);
			break;
		default:
			return -1;
	}
	printf("ant1_flags: %d, ant2_flags : %d, ant3_flags : %d, ant4_flags : %d\n",isant1,isant2,isant3,isant4);

	return 0;
}

int module_ant_judge(int ant1, int ant2, int ant3, int ant4)
{
	printf("ant1: %d, ant2 : %d, ant3 : %d, ant4 : %d\n",ant1,ant2,ant3,ant4);
	printf("ant1_flags: %d, ant2_flags : %d, ant3_flags : %d, ant4_flags : %d\n",isant1,isant2,isant3,isant4);
	int flags = 0;
	if(isant1 > 1)
		isant1 = 1;
	if(isant2 > 1)
		isant2 = 1;
	if(isant3 > 1)
		isant3 = 1;
	if(isant4 > 1)
		isant4 = 1;
	
	if(1 == ant1 )  {
		if(1 == isant1)
			flags = 1;
		else
			flags = 0;
	}

	if(1 == ant2 )  {
		if(1 == isant2)
			flags = 1;
		else 
			flags = 0;
	}

	if(1 == ant3 )  {
		if(1 == isant3)
			flags = 1;
		else
			flags = 0;
	}

	if(1 == ant4 )  {
		if(1 == isant4)
			flags = 1;
		else
			flags = 0;
	}

	return flags;
}

int module_ant_check()
{
	TMR_Status ret;
  	TMR_SR_PortDetect ports[TMR_SR_MAX_ANTENNA_PORTS];
 	TMR_SR_PortPair searchList[TMR_SR_MAX_ANTENNA_PORTS];
  	uint8_t i, listLen, numPorts;
  	uint16_t j;
  	TMR_AntennaMapList *map;
    
  	ret = TMR_SUCCESS;
  	map = rp->u.serialReader.txRxMap;

 	 /* 1. Detect current set of antennas */
	 int va = 1;
	 TMR_SR_cmdSetReaderConfiguration(rp, TMR_SR_CONFIGURATION_SAFETY_ANTENNA_CHECK, &va);
 	 numPorts = TMR_SR_MAX_ANTENNA_PORTS;
  	ret = TMR_SR_cmdAntennaDetect(rp, &numPorts, ports);
 	 if (TMR_SUCCESS != ret)
  	{
  		checkerr(rp, ret, 1, "TMR_SR_cmdAntennaDetect");
   		 return ret;
 	 }
	printf("numPorts : %d\n", numPorts);
  /* 2. Set antenna list based on detected antennas (Might be clever
   * to cache this and not bother sending the set-list command
   * again, but it's more code and data space).
   */
 	 for (i = 0, listLen = 0; i < numPorts; i++)
 	 {
 	 	printf("port %d, %d\n", i, ports[i].detected);
   		 if (ports[i].detected)
   		 {
     		 /* Ensure that the port exists in the map */
      			for (j = 0; j < map->len; j++) {
       			 if (ports[i].port == map->list[j].txPort)
       			 {
          				searchList[listLen].txPort = map->list[j].txPort;
         			 	searchList[listLen].rxPort = map->list[j].rxPort;
					module_ant_flags(searchList[listLen].txPort);
          				listLen++;
          				break;
        			}
      			}
   		 }
  	}

   	antlen = listLen;
 
/*
  	ret = TMR_SR_cmdSetAntennaSearchList(rp, listLen, searchList);
  */
 	 return ret;
}


int module_get_ant(uint8_t *ant , uint8_t *antennaCount)
{
	int ant1;
	sysSettingGetInt("ant1", &ant1, 1);
	int ant2;
	sysSettingGetInt("ant2", &ant2, 1);
	int ant3;
	sysSettingGetInt("ant3", &ant3, 1);
	int ant4;
	sysSettingGetInt("ant4", &ant4, 1);
	if(!ant1 && !ant2 && !ant3 && !ant4)
		return -1;

	int i = 0, j;
	if(1 == ant1) {
		ant[i] = 1;
		i++;
	}

	if(1 == ant2) {
		ant[i] = 2;
		i++;
	}

	if(1 == ant3) {
		ant[i] = 3;
		i++;
	}
	if(1 == ant4) {
		ant[i] = 4;
		i++;
	}
	*antennaCount = i;
	return 0;
}

void
printPortValueList(TMR_PortValueList *list)
{
  int i;

  putchar('[');
  for (i = 0; i < list->len && i < list->max; i++)
  {
    printf("[%u,%u]%s", list->list[i].port, list->list[i].value,
           ((i + 1) == list->len) ? "" : ",");
  }
  if (list->len > list->max)
  {
    printf("...");
  }
  putchar(']');
}

void module_ant_power()
{
	printf("ant1_power begin\n");
	int ret;
	TMR_PortValueList list;
	list.len = 4;
  	list.max = 4;
  	list.list = malloc(sizeof(list.list[0]) * list.max);
	if(NULL == list.list)
		return;
	int ant1_power;
	sysSettingGetInt("ant1_power", &ant1_power, 1);
	int ant2_power;
	sysSettingGetInt("ant2_power", &ant2_power, 1);
	int ant3_power;
	sysSettingGetInt("ant3_power", &ant3_power, 1);
	int ant4_power;
	sysSettingGetInt("ant4_power", &ant4_power, 1);

	list.list[0].port = 1;
	list.list[0].value = ant1_power;

	list.list[1].port = 2;
	list.list[1].value = ant2_power;

	list.list[2].port = 3;
	list.list[2].value = ant3_power;

	list.list[3].port = 4;
	list.list[3].value = ant4_power;

	ret = TMR_paramSet(rp, TMR_PARAM_RADIO_PORTREADPOWERLIST, &list);
    	free(list.list);
}

int module_init()
{
	TMR_Status ret;
	rp = &r;
	ret = TMR_create(rp, DEVICE_NAME);
	checkerr(rp, ret, 1, "creating reader");
	if(ret != 0)
		goto Err;

	ret = TMR_connect(rp);
	checkerr(rp, ret, 1, "connecting reader");
	if(ret != 0)
		goto Err;

	TMR_String model;
	TMR_paramGet(rp, TMR_PARAM_VERSION_MODEL, &model);

	region = TMR_REGION_NA;
	ret = TMR_paramSet(rp, TMR_PARAM_REGION_ID, &region);
	checkerr(rp, ret, 1, "setting region");
	if(ret != 0)
		goto Err;
	/*TMR_uint8List value;
       uint8_t valueList[64];

       value.max = numberof(valueList);
       value.list = valueList;
	TMR_SR_paramGet(rp, TMR_PARAM_ANTENNA_CONNECTEDPORTLIST, &value);
	*/
	sysSettingSetInt("ant1", 0);
	sysSettingSetInt("ant2", 0);
	sysSettingSetInt("ant3", 0);
	sysSettingSetInt("ant4", 0);
	module_ant_check();

	g_mutex = mid_mutex_create();
      if(g_mutex == NULL) {
          printf("mid_mutex_create");
          return -1;
      }

	//sysSettingGetInt(const char * name, int * value, int searchFlag)
/*	if(0 == strcmp("M6e Micro", model.value) || 0 == strcmp("M6e", model.value))
	{
		region = TMR_REGION_NA;
		ret = TMR_paramSet(rp, TMR_PARAM_REGION_ID, &region);
		checkerr(rp, ret, 1, "setting region");
		if(ret != 0)
			goto Err;
	} else if(0 == strcmp("M6e Nano", model.value)) {
		region = TMR_REGION_NA2;
		ret = TMR_paramSet(rp, TMR_PARAM_REGION_ID, &region);
		checkerr(rp, ret, 1, "setting region");
		if(ret != 0)
			goto Err;
	} else {
		region = TMR_REGION_NA;
		ret = TMR_paramSet(rp, TMR_PARAM_REGION_ID, &region);
		checkerr(rp, ret, 1, "setting region");
		if(ret != 0)
			goto Err;		
	}*/

	return 0;
Err:
	printf("module init failed!\n");
	return -1;
}

int module_clear()
{
	TMR_Status ret;
	ret = TMR_init_UserConfigOp(&config, TMR_USERCONFIG_CLEAR);
	checkerr(rp, ret, 1, "Initialization configuration: reset all saved configuration params");
	ret = TMR_paramSet(rp, TMR_PARAM_USER_CONFIG, &config);
	checkerr(rp, ret, 1, "setting user configuration option: reset all configuration parameters");
	printf("User config set option:reset all configuration parameters\n");

}

/*int module_gpitriger()
{
	TMR_Status ret;
	TMR_GEN2_Bank bank = TMR_GEN2_BANK_TID;
	int isReadData = 0;
	uint32_t wordAddres = 0;
	uint8_t len= 0;
	uint8_t *antennaList = NULL;
	uint8_t buffer[20] = {1};
	uint8_t antennaCount = 1;
	antennaList = buffer;
	
	TMR_RP_init_simple(&plan, antennaCount, antennaList, TMR_TAG_PROTOCOL_GEN2, 1000);	

	if(isReadData) {
		ret = TMR_TagOp_init_GEN2_ReadData(&op, bank, wordAddres, len);
		checkerr(rp, ret, 1, "creating tagop: GEN2 read data");
		ret = TMR_RP_set_tagop(&plan, &op);
		checkerr(rp, ret, 1, "setting tagop");
	} else {
		ret = TMR_RP_set_tagop(&plan, NULL);
		checkerr(rp, ret, 1, "setting tagop");		
	}

	ret = TMR_GPITR_init_enable (&triggerRead, true);
	checkerr(rp, ret, 1, "Initializing the trigger read");
	ret = TMR_RP_set_enableTriggerRead(&plan, &triggerRead);
	checkerr(rp, ret, 1, "setting trigger read");

	gpiPort.len = gpiPort.max = 1;
	gpiPort.list = data;
	ret = TMR_paramSet(rp, TMR_PARAM_TRIGGER_READ_GPI, &gpiPort);
	checkerr(rp, ret, 1, "setting GPI port");
	ret = TMR_RP_set_enableAutonomousRead(&plan, true);
	checkerr(rp, ret, 1, "setting autonomous read");


	ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &plan);
	checkerr(rp, ret, 1, "setting read plan");

	int readpower = 500;
	sysSettingGetInt("readpower", &readpower ,1);
	ret = TMR_paramSet(rp, TMR_PARAM_RADIO_READPOWER,&readpower);

	TMR_GEN2_Session session;
	session = TMR_GEN2_SESSION_S1;
 	ret = TMR_paramSet(rp,TMR_PARAM_GEN2_SESSION, &session);
	checkerr(rp, ret, 1, "setting session");

	
	ret = TMR_init_UserConfigOp(&config, TMR_USERCONFIG_SAVE_WITH_READPLAN);
	checkerr(rp, ret, 1, "Initializing user configuration: save read plan configuration");
	ret = TMR_paramSet(rp, TMR_PARAM_USER_CONFIG, &config);
	checkerr(rp, ret, 1, "setting user configuration: save read plan configuration");
	printf("User config set option:save read plan configuration\n");

	ret = TMR_init_UserConfigOp(&config, TMR_USERCONFIG_RESTORE);
	checkerr(rp, ret, 1, "Initialization configuration: restore all saved configuration params");
	ret = TMR_paramSet(rp, TMR_PARAM_USER_CONFIG, &config);
	checkerr(rp, ret, 1, "setting configuration: restore all saved configuration params");
	printf("User config set option:restore all saved configuration params\n");

	rlb.listener = callback;
	rlb.cookie = NULL;

	reb.listener = exceptionCallback;
	reb.cookie = NULL;

	ret = TMR_addReadListener(rp, &rlb);
	checkerr(rp, ret, 1, "adding read listener");

	ret = TMR_addReadExceptionListener(rp, &reb);
      checkerr(rp, ret, 1, "adding exception listener");

	ret = TMR_receiveAutonomousReading(rp, NULL, NULL);
	checkerr(rp, ret, 1, "Autonomous reading");	
}
*/
void socket_data_close()
{
	close(socket_data);
	return;
}


void mid_tagdata_nocache(char *epcstr, char *time, int ant)
{
	char str[128];
	int ret = -1;
	if(socket_data < 0) {
		socket_data = connect_server_data();
		if(socket_data < 0) 
			return -1;
	}
	memset(str, 0 , 128);
	sprintf(str, "%s,%s,%d,%d;", epcstr, time, 1, ant);
	ret = mid_tagdata_server(str, 1, socket_data);
	if(ret < 0) {
		close(socket_data);
		socket_data = -1;
	}
}

void mid_tagdata_cache(int flags)
{
	int cache = 0;
	int number,time;
	printf("mid_tagdata_cache****************************************\n");
	sysSettingGetInt("enable_cache", &cache, 1);
	int count = uniqueCount;
	if(count <= 0) 
		goto END;
	module_number_get(&number);
	module_number_set(number +1);		
	sqlite_insert_number(number+1);
	db_free();
	uniqueCount = 0;	

	int socket_fd;
	socket_fd = connect_server_data();
	if(socket_fd > 0) 
		create_sqlite_number(socket_fd, number +1);
		
END:
	close(socket_fd);
	sysSettingGetInt("time",  &time, 1);
	if(time < 2000)
		mid_timer_create(2 ,1, (mid_timer_f)mid_tagdata_cache, 0);
	else 
		mid_timer_create(time/1000 ,1, (mid_timer_f)mid_tagdata_cache, 0);

}

void mid_tagdata_send_v(int flag)
{
	int number;
	int time;
	//sysSettingGetInt("number", &number, 1);
	//sysSettingSetInt("number", number +1);  //¸ü¸Änumber
	//sys_config_save();
	int cache = 0;
	sysSettingGetInt("enable_cache", &cache, 1);
	int count = uniqueCount;
	if(count <= 0) {
		sysSettingGetInt("time",  &time, 1);
		mid_timer_create(time/1000,1, (mid_timer_f)mid_tagdata_send_v, 0);
		return -1;
	}
		
	int socket_fd;
	socket_fd = connect_server_data();
	if(socket_fd <= 0) {
		if(cache) {
			module_number_get(&number);
			module_number_set(number +1);		
			sqlite_insert_number(number+1);
		}
		db_free();
		uniqueCount = 0;		
		printf("tmr_file_read##########\n");
		goto END;
	}
	

	printf("uniqueCount : %d\n", count);

	if(1 == flag) {
		db_free();
		uniqueCount = 0;	
		sysSettingSetInt("enable_cache", 0);
		
		module_number_get(&number);
		create_sqlite_number(socket_fd, number +1);
		module_number_set(number +1);
	} else if(2 == flag) {
		if(0 == count)
			goto END;
		char *str = (char *)malloc(count * 64);
		memset(str, 0, count * 64);
		db_lookup_string(str, count);
		db_free();
		uniqueCount = 0;
		sysSettingSetInt("enable_cache", 1);
		module_number_get(&number);
		sqlite_insert_number(number);
		if(mid_tagdata_server(str, count, socket_fd) < 0)
		{
			printf("send filed!\n");
			free(str);
			goto END;
		} else {
			socket_receive_ex(3000, socket_fd);
			free(str);
			goto END;
		}
	}else {
		if(!cache) {
			if(0 == count)
				goto END;
			char *str = (char *)malloc(count * 64);
			memset(str, 0, count * 64);
			printf("db_lookup_string\n");
			db_lookup_string(str, count);
			db_free();
			uniqueCount = 0;
			if(mid_tagdata_server(str, count, socket_fd) < 0)
			{
				printf("send filed!\n");
				free(str);
				goto END;
			} else {
				socket_receive_ex(3000, socket_fd);
				free(str);
				goto END;
			}		
		} else {
			module_number_get(&number);
			module_number_set(number +1);		
			sqlite_insert_number(number+1);
			db_free(number+1);
			uniqueCount = 0;		
			create_sqlite_number(socket_fd, number +1);	
		}		
	}

END:
	mid_mutex_unlock(g_mutex);
	close(socket_fd);
	sysSettingGetInt("time",  &time, 1);
	if(time < 1000)
		mid_timer_create(1 ,1, (mid_timer_f)mid_tagdata_send_v, 0);
	else 
		mid_timer_create(time/1000,1, (mid_timer_f)mid_tagdata_send_v, 0);
		
}

void module_startReading()
{
	printf("module_startReading begin\n");

	int isStart,ret, i;
	uint8_t buffer[20] = {1,4};
	printf("buffer = %d\n", strlen(buffer));
	uint8_t *antennaList1 = NULL;
	uint8_t antennaCount1 = 0;
	//antennaList = (uint *)malloc(10);;
	if(0 == antlen)
		while(1);

	rlb.listener = callback;
	rlb.cookie = NULL;

	reb.listener = exceptionCallback;
	reb.cookie = NULL;

  	TMR_addReadListener(rp, &rlb);
  	//heckerr(rp, ret, 1, "adding read listener");

 	 TMR_addReadExceptionListener(rp, &reb);
 	 //checkerr(rp, ret, 1, "adding exception listener");
	hash_init();
	//sysSettingGetInt("tagcount", &uniqueCount, 1);
	int cache = 0;
	sysSettingGetInt("enable_cache", &cache, 1);
	if(cache) {
		int time;
		sysSettingGetInt("time",  &time, 1);
		mid_timer_create(time/1000 + 2 , 1, (mid_timer_f)mid_tagdata_cache, 0);
	}
	
	TMR_GEN2_Session session = TMR_GEN2_SESSION_S1;
 	ret = TMR_paramSet(rp,TMR_PARAM_GEN2_SESSION, &session);
	checkerr(rp, ret, 1, "setting session");
	//mid_timer_create(2, 1, (mid_timer_f)sqlite_insert_v, 0);

	while(1) {
		module_send_get(&isStart);
		printf("isstart : %d\n", isStart);
		//sysSettingGetInt("enable_cache", &cache, 1);
		if(isStart) {
			antennaCount = 0;
			memset(data, 0, 10);
			ret = module_get_ant(data, &antennaCount);
			if(!ret) 
				antennaList = data;

			TMR_RP_init_simple(&plan, antennaCount, antennaList, TMR_TAG_PROTOCOL_GEN2, 1000);	
			ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &plan);
			checkerr(rp, ret, 1, "TMR_PARAM_READ_PLAN");

			module_ant_power();


			ret = TMR_startReading(rp);
			printf("TMR_startReading\n");
			checkerr(rp, ret, 1, "startreading");
			//setTimer(2, 0);
			sleep(3);
			ret = TMR_stopReading(rp);
			printf("TMR_stopReading\n");
			checkerr(rp, ret, 1, "stopreading");
		}
		setTimer(0, 100*1000);
	}
	printf("module_startReading end\n");
}


/*void mid_tagdata_send(int flag)
{
	int socket_fd;
	socket_fd = connect_server_data();
	if(socket_fd <= 0)
		goto END;
	 if(mid_mutex_lock(g_mutex)){
            printf("mutex lock error. Get doubleStack.\n");
            goto END;
       }
	
	int cache = 0;
	sysSettingGetInt("enable_cache", &cache, 1);
	printf("uniqueCount : %d\n", uniqueCount);
	char *str = (char *)malloc(uniqueCount * 24);
	memset(str, 0, uniqueCount * 24);
	if(flag || !cache) {
		printf("db_lookup_string\n");
		db_lookup_string(seenTags, str);
		db_free(seenTags);
		uniqueCount = 0;
		seenTags = init_tag_database(db_size);
		if(mid_tagdata_server(str, uniqueCount, socket_fd) < 0)
		{
			printf("send filed!\n");
			free(str);
			goto END;
		} else {
			socket_receive_ex(3000, socket_fd);
			free(str);
			goto END;
		}		
	} else {
		if(tmr_file_read(socket_fd) == 0)
		{
			printf("tmr_file_read\n");
			sysSettingSetInt("tagcount", 0);
			db_free(seenTags);
			uniqueCount = 0;		
			seenTags = init_tag_database(db_size);
			printf("tmr_file_read##########\n");

		}
		free(str);
	}

END:
	mid_mutex_unlock(g_mutex);
	int time;
	close(socket_fd);
	sysSettingGetInt("time",  &time, 1);
	mid_timer_create(time/1000,1, (mid_timer_f)mid_tagdata_send, 0);
}
*/
void
callback(TMR_Reader *reader, const TMR_TagReadData *t, void *cookie)
{
	char epcStr[128];
	char time[24];
	TMR_bytesToHex(t->tag.epc, t->tag.epcByteCount, epcStr);
	//printf("epc: %s, ant : %d\n", epcStr, t->antenna);
	sys_get_time(time);
	int cache = 0;
	sysSettingGetInt("enable_cache", &cache, 1);
	if(cache) {
		if ( NULL == db_lookup(epcStr) && uniqueCount < 1000) {
			uniqueCount ++;
			db_insert(epcStr, time);
			sqlite_insert_data(epcStr, time);
			//enqueue_p(epcStr, time);
		}
	} else {
		mid_tagdata_nocache(epcStr, time, t->antenna);
	}

}

void reboot_connect()
{
	printf("reboot_connect!\n");
}

void 
exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie)
{
  	if(TMR_ERROR_TIMEOUT) {
  	  	//fprintf(stdout, "Error:%s\n", TMR_strerr(reader, error));
  	}  else if(error == TMR_ERROR_NOT_FOUND || error == TMR_ERROR_NO_TAGS || error == TMR_ERROR_NO_TAGS_FOUND) {
	  	fprintf(stdout, "Error:%s\n", TMR_strerr(reader, error));
	 } else {
  		printf("Error:%s\n",TMR_strerr(reader, error));
  		//printf("reboot!\n");
  	}
}


void module_detroy()
{
	printf("module_detroy!\n");
	TMR_Status ret;
	ret = TMR_removeReadListener(rp, &rlb);
	checkerr(rp, ret, 1, "remove read listener");

	ret = TMR_removeReadExceptionListener(rp,&reb);
	checkerr(rp, ret, 1, "remove read ExceptionListener");
	TMR_destroy(rp);
}

