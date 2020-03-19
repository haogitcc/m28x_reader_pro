#include "m6e_init.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "mid_task.h"
#include "include/mercuryapi_include/tm_reader.h"
#include "include/mercuryapi_include/tmr_utils.h"

TMR_Reader r,*rp;
TMR_ReadPlan plan;
int antennaCount = 0;
uint8_t *antennaList = NULL;

static pthread_cond_t cond;
static pthread_mutex_t mutex;

int m6e_init(char* string)
{
    TMR_Status ret;

	int region = 1;


	rp = &r;

	ret = TMR_create(rp, string);

	if(ret != TMR_SUCCESS)

	{

		printf("Create device failed!\n");

		return -1;

	}

	printf("create success\n");



	ret = TMR_connect(rp);

	if(ret != TMR_SUCCESS)

	{

		printf("Open device failed!\n");

		return -1;

	}

	printf("connect success\n");

	

	TMR_RP_init_simple(&plan, antennaCount, antennaList, TMR_TAG_PROTOCOL_GEN2, 1000);

	TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &plan);



	pthread_mutex_init(&mutex,NULL);

	pthread_cond_init(&cond, NULL);

	return ret;

}

int set_bundrate(int rate)

{

    int ret = -1;

	ret = TMR_paramSet(rp,TMR_PARAM_BAUDRATE,&rate);

	return ret;

}

int m6e_destory()

{

   int ret = -1;

   ret = TMR_destroy(rp);

   return ret;

}

int m6e_set_configuration(int key, int value)

{

	int ret = -1;

	ret = TMR_paramSet(rp,key, &value);

	return ret;

}
