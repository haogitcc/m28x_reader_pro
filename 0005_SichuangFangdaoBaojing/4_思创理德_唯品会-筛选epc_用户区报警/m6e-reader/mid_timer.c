
#include <time.h>
#include <sys/time.h>

#include "net_include.h"
#include "mid_timer.h"
#include "mid_mutex.h"
#include "mid_msgq.h"
#include "mid_task.h"

#define TIMER_POOL_SIZE		128

#define HOUR_CLOCK_NUM		(60*60*100)
#define YEAR_CLOCK_NUM		(365*60*60*100)
#define ERR_OUT() \
do{                                     \
    printf("error!\n");  \
    goto Err;                           \
} while(0)


typedef struct _msg_timer_t msg_timer_t;
typedef struct _msg_timer_q msg_timer_q;
struct _msg_timer_t
{
	msg_timer_t *next;
	mid_timer_f func;
	int arg;
	unsigned int loops;
	unsigned int intval;
	unsigned int clock;
};
struct _msg_timer_q
{
	msg_timer_t *head;
};

static msg_timer_q	g_timer_q_pool;
static msg_timer_q	g_timer_q_time;


static mid_timer_f	g_deal_funcs[TIMER_POOL_SIZE];
static int			g_deal_args[TIMER_POOL_SIZE];
static int			g_deal_num = 0;

static mid_mutex_t	g_mutex = NULL;
static mid_msgq_t	g_msgq = NULL;

static void msg_timer_queue(msg_timer_t *timer, unsigned int pClock)
{
	msg_timer_t *next, *prev;

	while(timer->clock <= pClock && timer->intval > 0)
		timer->clock += timer->intval;

	next = g_timer_q_time.head;
	prev = NULL;
	while(next && next->clock <= timer->clock) {
		prev = next;
		next = next->next;
	}

	timer->next = next;
	if (prev)
		prev->next = timer;
	else
		g_timer_q_time.head = timer;
}

static msg_timer_t *msg_timer_dequeue(mid_timer_f func, int arg, int any)
{
	msg_timer_t *timer, *prev;

	timer = g_timer_q_time.head;
	prev = NULL;
	while(timer && (timer->func != func || (any == 0 && timer->arg != arg))) {
		prev = timer;
		timer = timer->next;
	}
	if (timer == NULL)
		return NULL;
	if (prev)
		prev->next = timer->next;
	else
		g_timer_q_time.head = timer->next;

	return timer;
}

unsigned int mid_10ms(void)
{
    unsigned int clk;
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);

    clk = (unsigned int)tp.tv_sec * 100 + (unsigned int)tp.tv_nsec / 10000000;

    return clk;
}


static msg_timer_t *msg_timer_alloc(void)
{
	msg_timer_t *timer = g_timer_q_pool.head;
	if (timer == NULL)
		return NULL;
	g_timer_q_pool.head = timer->next;
	timer->next = NULL;
	return timer;
}

static void msg_timer_free(msg_timer_t *timer)
{
	if (timer == NULL)
		return;
	timer->next = g_timer_q_pool.head;
	g_timer_q_pool.head = timer;
}

static unsigned int msg_timer_deal(void)
{
	unsigned int i;
	unsigned int tClock, remain;
	msg_timer_t *timer;
	mid_timer_f func;
	int arg;

	mid_mutex_lock(g_mutex);
	g_deal_num = 0;
	tClock = mid_10ms( );

	timer = g_timer_q_time.head;
	while(timer && timer->clock <= tClock) {
		g_deal_funcs[g_deal_num] = timer->func;
		g_deal_args[g_deal_num] = timer->arg;
		g_deal_num ++;
		msg_timer_dequeue(timer->func, timer->arg, 0);
		if (timer->loops == 0) {
			msg_timer_queue(timer, tClock);
		} else {
			timer->loops --;
			if (timer->loops > 0)
				msg_timer_queue(timer, tClock);
			else
				msg_timer_free(timer);
		}
		//记住 下面一行替换成timer = timer->next是错误的
		timer = g_timer_q_time.head;
	}
	timer = g_timer_q_time.head;
	if (timer)
		remain = timer->clock - tClock;
	else
		remain = HOUR_CLOCK_NUM;

	for (i = 0; i < g_deal_num; i ++) {
		func = g_deal_funcs[i];
		arg = g_deal_args[i];
		if (func) {
			//LogUserOperDebug("func = %p, arg = %d/0x%x\n", func, arg, arg);
			mid_mutex_unlock(g_mutex);
			//LogUserOperDebug("--------: func func = %p, arg = %d enter \n", func, arg);
			func(arg);
			//LogUserOperDebug("--------: func exit\n");
			mid_mutex_lock(g_mutex);
		}
	}
	g_deal_num = 0;

	mid_mutex_unlock(g_mutex);

	if (remain > HOUR_CLOCK_NUM)
		return HOUR_CLOCK_NUM;

	return remain;
}

void mid_timer_refresh(void)
{
	char ch;
	mid_msgq_putmsg(g_msgq, &ch);
}

/*
	intval: 间隔秒数
	loops：定时器执行循环次数
	func：定时到执函数
	arg：定时器到时传给func的参数
	一个定时器由连个参数标示func和arg
	在定时器执行的那个函数里不要干太多的事，最好是只给你自己任务发一个消息而已。
 */
int int_timer_create(int intval, int loops, mid_timer_f func, int arg)
{
	msg_timer_t *timer;
	unsigned int tClock;

	if (intval < 0 || loops < 0 || (intval == 0 && loops != 1) || func == NULL) {
		printf("param error! \n");
		goto Err;
	}
	mid_mutex_lock(g_mutex);

	timer = msg_timer_dequeue(func, arg, 0);
	if (timer == NULL)
		timer = msg_timer_alloc( );
	if (timer == NULL) {
		mid_mutex_unlock(g_mutex);
		printf("timer pool is empty!\n");
		goto Err;
	}
	tClock = mid_10ms( );
	printf("\n++++++++intval is: %d, func is: %p\n", intval, func);
	//LogUserOperDebug("++++++++ %d  %d %p\n", intval, msec, func);
	timer->intval = intval;
	timer->clock = tClock;
	timer->loops = loops;
	timer->func = func;
	timer->arg = arg;
	msg_timer_queue(timer, tClock);

	mid_mutex_unlock(g_mutex);
	printf("\nfunc = %p, arg = %d/0x%x\n", func, arg, arg);

	mid_timer_refresh( );

	return 0;
Err:
	return -1;
}

int mid_timer_micro(int intval, mid_timer_f func, int arg)
{
	return int_timer_create(intval / 10, 1, func, arg);
}

int mid_timer_create(int intval, int loops, mid_timer_f func, int arg)
{
	return int_timer_create(intval * 100, loops, func, arg);
}

void mid_timer_print(void)
{
	msg_timer_t *timer;

	printf("TIMER:\n");
	mid_mutex_lock(g_mutex);
	timer = g_timer_q_time.head;
	while(timer) {
		printf("%p: func = %p, arg = %d/0x%x\n", timer, timer->func, timer->arg, timer->arg);
		timer = timer->next;
	}
	mid_mutex_unlock(g_mutex);
}

void mid_timer_delete_all(mid_timer_f func)
{
	int i;
	msg_timer_t *timer;

	mid_mutex_lock(g_mutex);

	for (;;) {
		timer = msg_timer_dequeue(func, 0, 1);
		if (timer == NULL)
			break;
		msg_timer_free(timer);
	}
	if (g_deal_num > 0) {
		for (i = 0; i < g_deal_num; i ++) {
			if (g_deal_funcs[i] == func)
				g_deal_funcs[i] = NULL;
		}
	}

	mid_mutex_unlock(g_mutex);
	printf("func = %p\n", func);
}

void mid_timer_delete(mid_timer_f func, int arg)
{
	int i;
	msg_timer_t *timer;

	mid_mutex_lock(g_mutex);

	timer = msg_timer_dequeue(func, arg, 0);
	if (timer)
		msg_timer_free(timer);
	if (g_deal_num > 0) {
		for (i = 0; i < g_deal_num; i ++) {
			if (g_deal_funcs[i] == func && g_deal_args[i] == arg) {
				printf("func = %p, arg = %d/0x%x\n", func, arg, arg);
				g_deal_funcs[i] = NULL;
			}
		}
	}

	mid_mutex_unlock(g_mutex);
}

static void mid_timer_loop(void *arg)
{
	unsigned int clk;

	for (;;)
	{
		clk = msg_timer_deal( );
		mid_select_exec(clk);
	}
}

static void mid_timer_select(int flag)
{
	char ch;
	mid_msgq_getmsg(g_msgq, &ch);
}

void mid_timer_init(void)
{
	int i;

	printf("SIZE: _msg_timer_t = %d\n", sizeof(struct _msg_timer_t));

	mid_select_init( );

	g_timer_q_pool.head = NULL;
	g_timer_q_time.head = NULL;

	for (i = 0; i < TIMER_POOL_SIZE; i ++)
		msg_timer_free((msg_timer_t *)malloc(sizeof(msg_timer_t)));

	g_mutex = mid_mutex_create( );
	g_msgq = mid_msgq_create(16, 1);

	mid_select_regist(mid_msgq_fd(g_msgq), mid_timer_select, 0);

	//mid_msg_task_create(mid_timer_loop);
	mid_task_create("mid_msg", (mid_func_t)mid_timer_loop, 0);
}
