#include "tm_reader.h"
#include "serial_reader_imp.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include "gpio_init.h"
#include "mid_timer.h"
#include "server_m6e.h"
#include "module_init.h"

TMR_Reader r, *rp;
TMR_Region region = TMR_REGION_NA;
TMR_SR_UserConfigOp config;
TMR_GPITriggerRead triggerRead;
TMR_ReadPlan plan;
//uint8_t *antennaList = NULL;
//uint8_t antennaCount = 0;
static int length=0;
static char filter[64];
static char id[64];

TMR_TagOp op;
TMR_uint8List gpiPort;
uint8_t data[1] = {4};
TMR_SR_UserConfigOp config;

TMR_ReadListenerBlock rlb;
TMR_ReadExceptionListenerBlock reb;


#define DEVICE_NAME "tmr:///dev/ttySP0"

void callback(TMR_Reader *reader, const TMR_TagReadData *t, void *cookie);
void exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie);
void module_detroy();

void checkerr(TMR_Reader* rp, TMR_Status ret, int exitval, const char *msg)
{
	if (TMR_SUCCESS != ret)
	{
		//errx(exitval, "Error %s: %s\n", msg, TMR_strerr(rp, ret));
		printf("Error %s: %s\n",msg, TMR_strerr(rp, ret));
	}
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
	// int va = 1;
	// TMR_SR_cmdSetReaderConfiguration(rp, TMR_SR_CONFIGURATION_SAFETY_ANTENNA_CHECK, &va);
 	 numPorts = TMR_SR_MAX_ANTENNA_PORTS;
  	ret = TMR_SR_cmdAntennaDetect(rp, &numPorts, ports);
 	 if (TMR_SUCCESS != ret)
  	{
  		checkerr(rp, ret, 1, "TMR_SR_cmdAntennaDetect");
   		 return ret;
 	 }
  /* 2. Set antenna list based on detected antennas (Might be clever
   * to cache this and not bother sending the set-list command
   * again, but it's more code and data space).
   */
 	 for (i = 0, listLen = 0; i < numPorts; i++)
 	 {
   		 if (ports[i].detected)
   		 {
     		 /* Ensure that the port exists in the map */
      			for (j = 0; j < map->len; j++) {
       			 if (ports[i].port == map->list[j].txPort)
       			 {
          				searchList[listLen].txPort = map->list[j].txPort;
         			 	searchList[listLen].rxPort = map->list[j].rxPort;
					//module_ant_flags(searchList[listLen].txPort);
          				listLen++;
          				break;
        			}
      			}
   		 }
  	}

   	//antlen = listLen;
   	if(listLen == 0) {
		printf("no antenna!\n");
		return -1;
	}
 

  	ret = TMR_SR_cmdSetAntennaSearchList(rp, listLen, searchList);
  
 	 return ret;
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

int module_gpitriger()
{
	TMR_Status ret;
	TMR_GEN2_Bank bank = TMR_GEN2_BANK_TID;
	int isReadData = 0;
	uint32_t wordAddres = 0;
	uint8_t len= 0;
	uint8_t *antennaList = NULL;
	uint8_t buffer[20] = {1,2,3,4};
	uint8_t antennaCount = 4;
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

/*	ret = TMR_GPITR_init_enable (&triggerRead, true);
	checkerr(rp, ret, 1, "Initializing the trigger read");
	ret = TMR_RP_set_enableTriggerRead(&plan, &triggerRead);
	checkerr(rp, ret, 1, "setting trigger read");

	gpiPort.len = gpiPort.max = 1;
	gpiPort.list = data;
	ret = TMR_paramSet(rp, TMR_PARAM_TRIGGER_READ_GPI, &gpiPort);
	checkerr(rp, ret, 1, "setting GPI port");*/
	ret = TMR_RP_set_enableAutonomousRead(&plan, true);
	checkerr(rp, ret, 1, "setting autonomous read");

	/* Commit read plan */
	ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &plan);
	checkerr(rp, ret, 1, "setting read plan");
	if(ret != 0)
		goto Err;
	
	//int readpower = 500;
	//sysSettingGetInt("readpower", &readpower ,1);
	//ret = TMR_paramSet(rp, TMR_PARAM_RADIO_READPOWER,&readpower);
	//if(ret != 0)
	//	goto Err;
	TMR_PortValueList value;
	TMR_PortValueList *list;
	list = &value;
	 list->len = 4;
 	 list->max = 4;
 	 list->list = malloc(sizeof(list->list[0]) * list->max);
	 list->list[0].port = 1;
	 list->list[0].value = 3100;
	 list->list[1].port = 2;
	 list->list[1].value = 2900;
	 list->list[2].port = 3;
	 list->list[2].value = 2900;
	 list->list[3].port = 4;
	 list->list[3].value = 2900;
	 ret = TMR_paramSet(rp, TMR_PARAM_RADIO_PORTREADPOWERLIST, &value);
	 free(value.list);
		
	TMR_GEN2_LinkFrequency blf = TMR_GEN2_LINKFREQUENCY_250KHZ;
	ret = TMR_paramSet(rp,TMR_PARAM_GEN2_BLF, &blf);
	if(ret != 0)
		goto Err;

	TMR_GEN2_Tari tari = TMR_GEN2_TARI_25US;
	ret = TMR_paramSet(rp,TMR_PARAM_GEN2_TARI, &tari);
	if(ret != 0)
		goto Err;

	TMR_GEN2_Target target = TMR_GEN2_TARGET_A;
	ret = TMR_paramSet(rp,TMR_PARAM_GEN2_TARGET, &target);
	if(ret != 0)
		goto Err;	
	
	TMR_GEN2_Session session;
	session = TMR_GEN2_SESSION_S1;
 	ret = TMR_paramSet(rp,TMR_PARAM_GEN2_SESSION, &session);
	checkerr(rp, ret, 1, "setting session");
	
	if(ret != 0)
		goto Err;
	
	/* Init UserConfigOp structure to save read plan configuration */
	ret = TMR_init_UserConfigOp(&config, TMR_USERCONFIG_SAVE_WITH_READPLAN);
	checkerr(rp, ret, 1, "Initializing user configuration: save read plan configuration");
	if(ret != 0)
		goto Err;
	
	ret = TMR_paramSet(rp, TMR_PARAM_USER_CONFIG, &config);
	checkerr(rp, ret, 1, "setting user configuration: save read plan configuration");
	printf("User config set option:save read plan configuration\n");
	if(ret != 0)
		goto Err;
	/* Restore the save read plan configuration */
	/* Init UserConfigOp structure to Restore all saved configuration parameters */
	ret = TMR_init_UserConfigOp(&config, TMR_USERCONFIG_RESTORE);
	checkerr(rp, ret, 1, "Initialization configuration: restore all saved configuration params");
	if(ret != 0)
		goto Err;
	ret = TMR_paramSet(rp, TMR_PARAM_USER_CONFIG, &config);
	checkerr(rp, ret, 1, "setting configuration: restore all saved configuration params");
	printf("User config set option:restore all saved configuration params\n");
	if(ret != 0)
		goto Err;

	rlb.listener = callback;
	rlb.cookie = NULL;

	reb.listener = exceptionCallback;
	reb.cookie = NULL;

	ret = TMR_addReadListener(rp, &rlb);
	checkerr(rp, ret, 1, "adding read listener");

	ret = TMR_addReadExceptionListener(rp, &reb);
    	checkerr(rp, ret, 1, "adding exception listener");
	if(ret != 0)
		goto Err;
	ret = TMR_receiveAutonomousReading(rp, NULL, NULL);
	checkerr(rp, ret, 1, "Autonomous reading");
	if(ret != 0)
		goto Err;

	return 0;
Err:
	printf("read failed!\n");
	return -1;
}

void
callback(TMR_Reader *reader, const TMR_TagReadData *t, void *cookie)
{
	char epcStr[128];
	int i = 0;
	TMR_bytesToHex(t->tag.epc, t->tag.epcByteCount, epcStr);
	printf("epc:%s, ant: %d, RSSI: %d\n", epcStr, t->antenna, t->rssi);


	if(t->tag.epcByteCount == length && !strncmp(filter, epcStr, 4)) 
	{
		char str[4];
		for(i = 20; i < 24; i ++)
			str[i-20] = epcStr[i];
		if(strncmp(id, str, 4))
			return;
		int isBuzzer = 0;
		sys_buzzer_get(&isBuzzer);
		if(0 == isBuzzer) {
			sys_buzzer_set(1);
			gpio_write_60_v("1");
			mid_timer_create(5, 1, (mid_timer_f)gpio_stop, 0);
		}
	}

	/*if(0 == length) {
		if(!strncmp(epcStr, filter, strlen(filter))) {
			gpio_write_60_v("1");
			mid_timer_delete((mid_timer_f)gpio_stop, 0);
			mid_timer_create(2, 1, (mid_timer_f)gpio_stop, 0);
		}
	} else {
		if(!strncmp(epcStr, filter, strlen(filter)) && (length == t->tag.epcByteCount)) {
			gpio_write_60_v("1");
			mid_timer_delete((mid_timer_f)gpio_stop, 0);
			mid_timer_create(2, 1, (mid_timer_f)gpio_stop, 0);
		}
	}*/
	
}

void reboot_connect()
{
	printf("reboot_connect!\n");
  	//TMR_SR_reboot(rp);
  	module_detroy();
  	if(module_init() == 0) {
  		module_gpitriger();
  	} else {
		if(module_init() == 0)
			module_gpitriger();
  	}

}

void 
exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie)
{
  //fprintf(stdout, "Error:%s\n", TMR_strerr(reader, error));
  //printf("Error:%s\n",TMR_strerr(reader, error));
  if(error == TMR_ERROR_NOT_FOUND || error == TMR_ERROR_NO_TAGS || error == TMR_ERROR_NO_TAGS_FOUND || TMR_ERROR_TIMEOUT)
  	return;
  else {
  	printf("Error:%s\n",TMR_strerr(reader, error));
  	printf("reboot!\n");
	system("reboot");
/*	pthread_t stbmonitor_pthread = 0;
	pthread_attr_t tattr;
	pthread_attr_init(&tattr);
	pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
	pthread_create(&stbmonitor_pthread, &tattr, reboot_connect, NULL);*/
  }
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
	int ant1_power = 3000;
	//sysSettingGetInt("ant1_power", &ant1_power, 1);
	int ant2_power = 2700;
	//sysSettingGetInt("ant2_power", &ant2_power, 1);
	int ant3_power = 2100;
	//sysSettingGetInt("ant3_power", &ant3_power, 1);
	int ant4_power = 2700;
	//sysSettingGetInt("ant4_power", &ant4_power, 1);

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


void module_startReading()
{
	printf("module_startReading begin\n");

	int isStart,ret, i;
	uint8_t *antennaList= NULL;
	int readpower = 500;

	uint8_t antennaCount = 0;
	sysSettingGetInt("epc_length", &length, 1);
	sysSettingGetString("filter", filter, 64, 1);
	sysSettingGetString("id", id, 64, 1);

	if(module_ant_check() == -1)
		return;
	TMR_RP_init_simple(&plan, antennaCount, antennaList, TMR_TAG_PROTOCOL_GEN2, 1000);	
	ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &plan);
	checkerr(rp, ret, 1, "TMR_PARAM_READ_PLAN");


	rlb.listener = callback;
	rlb.cookie = NULL;

	reb.listener = exceptionCallback;
	reb.cookie = NULL;

  	TMR_addReadListener(rp, &rlb);
  	//heckerr(rp, ret, 1, "adding read listener");

 	 TMR_addReadExceptionListener(rp, &reb);
	
	TMR_GEN2_Session session = TMR_GEN2_SESSION_S1;
 	ret = TMR_paramSet(rp,TMR_PARAM_GEN2_SESSION, &session);
	checkerr(rp, ret, 1, "setting session");
	//mid_timer_create(2, 1, (mid_timer_f)sqlite_insert_v, 0);

	while(1) {
		//module_send_get(&isStart);
		//printf("isstart : %d\n", isStart);
		//sysSettingGetInt("enable_cache", &cache, 1);
		//if(isStart) {

			//module_ant_power();

			sysSettingGetInt("readpower", &readpower ,1);
			ret = TMR_paramSet(rp, TMR_PARAM_RADIO_READPOWER,&readpower);

			ret = TMR_startReading(rp);
			printf("TMR_startReading\n");
			checkerr(rp, ret, 1, "startreading");
			//setTimer(2, 0);
			sleep(3);
			ret = TMR_stopReading(rp);
			printf("TMR_stopReading\n");
			checkerr(rp, ret, 1, "stopreading");
		//}
		setTimer(0, 200*1000);
	}
	printf("module_startReading end\n");
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

