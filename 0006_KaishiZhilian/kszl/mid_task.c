
//#include "mid_include.h"

#include <pthread.h>
#include <unistd.h>

#include "mid_task.h"
#include "mid_mutex.h"

typedef void *(*pthread_routine) (void *);

#define MID_TASKNUM_MAX		64

#define ERR_OUT() \
do{                                     \
    printf("error!\n");  \
    goto Err;                           \
} while(0)


static int g_index;
static pthread_t g_threads[MID_TASKNUM_MAX];

static mid_mutex_t g_mutex = NULL;

void mid_task_init(void)
{
	int i;

    if (g_mutex)
        return;
	g_mutex = mid_mutex_create( ); /*yanglw: 互斥量的初始化*/
	for(i = 0; i < MID_TASKNUM_MAX; i ++)
		g_threads[i] = -1;
	g_index = 0;
}

int mid_task_create(const char* name, mid_func_t func, void *arg)
{
	int tIndex, err;

	if(g_mutex == NULL) {
		printf("mid task not init\n");
		ERR_OUT();
	}
	if (mid_mutex_lock(g_mutex))
		printf("\n");
	if(g_index >= MID_TASKNUM_MAX) {
		mid_mutex_unlock(g_mutex);
		printf("mid task not init %d, %d, %s\n", g_index, MID_TASKNUM_MAX, name);
		ERR_OUT();
	}
	tIndex = g_index;
	g_index ++;
	mid_mutex_unlock(g_mutex);

	err = pthread_create(&g_threads[tIndex], NULL, (pthread_routine)func, arg);
	if (err) {
		printf("pthread_create\n");
		ERR_OUT();
	}

	return 0;
Err:
	return -1;
}


int mid_task_create_ex(const char* name, mid_func_t func, void *arg)
{
	int tIndex, err;

	if(g_mutex == NULL) {
		printf("mid task not init\n");
		ERR_OUT();
	}

	if (mid_mutex_lock(g_mutex))
		printf("\n");
	if(g_index >= MID_TASKNUM_MAX) {
		mid_mutex_unlock(g_mutex);
		printf("mid task not init %d, %d, %s\n", g_index, MID_TASKNUM_MAX, name);
		ERR_OUT();
	}
	tIndex = g_index;
	g_index ++;
	mid_mutex_unlock(g_mutex);

	pthread_attr_t attrThread;
//	pthread_t  epThread;
	struct sched_param param;

	pthread_attr_init(&attrThread);
	pthread_attr_setdetachstate(&attrThread, PTHREAD_CREATE_DETACHED);//这句是设置线程为非绑定，不是必须的
	pthread_attr_setschedpolicy(&attrThread, SCHED_FIFO);
	pthread_attr_getschedparam(&attrThread, &param);
	param.sched_priority = (sched_get_priority_max(SCHED_FIFO) + sched_get_priority_min(SCHED_FIFO)) / 2;
	pthread_attr_setschedparam(&attrThread, &param);
    /**不设置堆栈大小**/
	//pthread_attr_setstacksize(&attrThread, stack_size);

	err = pthread_create(&g_threads[tIndex], &attrThread, (pthread_routine)func, arg);
	if (err) {
		printf("pthread_create\n");
		ERR_OUT();
	}

	return 0;
Err:
	return -1;
}

void mid_task_delay(unsigned int ms)
{
    int i;
    for (i=0; i<ms; i++) {
        sched_yield();
        usleep(1000);
    }
	// usleep(ms * 1000);
}

