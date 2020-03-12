#include "tm_reader.h"
#include "serial_reader_imp.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include "gpio_init.h"


TMR_Reader r, *rp;
TMR_Region region = TMR_REGION_NA;
TMR_SR_UserConfigOp config;
TMR_GPITriggerRead triggerRead;
TMR_ReadPlan plan;
//uint8_t *antennaList = NULL;
//uint8_t antennaCount = 0;

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

int module_init()
{
	TMR_Status ret;
	TMR_Param param;

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

	int readpower = 3000;
	sysSettingGetInt("readpower", &readpower ,1);
	ret = TMR_paramSet(rp, TMR_PARAM_RADIO_READPOWER,&readpower);

	uint8_t in[] = {4};

      param = TMR_PARAM_GPIO_INPUTLIST;
      ret = TMR_paramSet(rp, param , &in_key);
      if (TMR_SUCCESS != ret)
      {
        errx(1, "Error setting gpio input list: %s\n", TMR_strerr(rp, ret));
      }
      else
      {
        printf("Input list set\n");
      }

	return 0;
Err:
	printf("module init failed!\n");
	return -1;
}

