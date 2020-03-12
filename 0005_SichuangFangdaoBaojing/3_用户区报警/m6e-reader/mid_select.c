#include <sys/select.h>

#include "net_include.h"
#include "mid_select.h"
#include "mid_mutex.h"

#define SELECT_NUM_MAX		16

static fd_set g_rset;

struct mid_select {
	struct mid_select *next;
	int fd;
	int arg;
	mid_select_f callback;
};

static struct mid_select *g_selarray = NULL;
static int g_selnum = 0;
unsigned int g_magic = 0;
static int g_maxfd = -1;

static mid_mutex_t g_mutex = NULL;

void mid_select_init(void)
{
	printf("SIZE: mid_select = %d\n", sizeof(struct mid_select));
	if (g_mutex)
		return;
	g_selarray = (struct mid_select *)malloc(sizeof(struct mid_select) * SELECT_NUM_MAX);
	if (g_selarray == NULL) {
		printf("malloc\n");
		goto Err;
	}
	g_mutex = mid_mutex_create( );
Err:
	return;
}

void mid_select_exec(unsigned int clk)
{
	int i, maxfd, selnum;
	fd_set rset;
	struct timeval tv;
	struct mid_select *sel;

	//DBG_PRN("clk = %d\n", clk);
	tv.tv_sec = clk / 100;
	tv.tv_usec = clk % 100 * 10000;

	mid_mutex_lock(g_mutex);
	if (g_maxfd == -1) {
		FD_ZERO(&g_rset);
		for (i = 0; i < g_selnum; i ++) {
			sel = &g_selarray[i];
			FD_SET(sel->fd, &g_rset);
			if (sel->fd > g_maxfd)
				g_maxfd = sel->fd;
			//DBG_PRN("[%d] fd = %d\n", i, sel.fd);
		}
	}
	maxfd = g_maxfd + 1;
	selnum = g_selnum;
	memcpy(&rset, &g_rset, sizeof(fd_set));
	mid_mutex_unlock(g_mutex);

	if (maxfd < 1 || selnum <= 0) {
		printf("maxfd = %d, selnum = %d\n", maxfd, selnum);
		goto Err;
	}
	selnum = select(maxfd, &rset, NULL, NULL, &tv);
	//DBG_PRN("selnum = %d, errno = %d\n", selnum, errno);
	if (selnum <= 0)
		return;

	mid_mutex_lock(g_mutex);
	for (i = 0; i < g_selnum; i ++) {
		int arg;
		mid_select_f callback;

		sel = &g_selarray[i];
		if (FD_ISSET(sel->fd, &rset)) {
			//DBG_PRN("[%d] fd = %d\n", i, sel.fd);
			callback = sel->callback;
			arg = sel->arg;
			mid_mutex_unlock(g_mutex);
			callback(arg);
			mid_mutex_lock(g_mutex);
			if (g_maxfd == -1)
				break;
		}
	}
	mid_mutex_unlock(g_mutex);

Err:
	return;
}

int mid_select_regist(int fd, mid_select_f callback, int arg)
{
	int i, ret = -1;
	struct mid_select *sel = NULL;

	mid_mutex_lock(g_mutex);

	for (i = 0; i < g_selnum; i ++) {
		if (g_selarray[i].fd == fd) {
			sel = &g_selarray[i];
			break;
		}
	}
	if (sel == NULL) {
		if (g_selnum >= SELECT_NUM_MAX) {
			printf("select array is full\n");
			goto Err;
		}
		sel = &g_selarray[g_selnum];
		sel->fd = fd;
		g_selnum ++;
	}
	sel->callback = callback;
	sel->arg = arg;
	g_maxfd = -1;

	ret = 0;
Err:
	mid_mutex_unlock(g_mutex);
	//DBG_PRN("ret = %d\n", ret);
	if (ret == 0)
		mid_timer_refresh( );

	return ret;
}

void mid_select_unregist(int fd)
{
	int i;

	mid_mutex_lock(g_mutex);
	if (g_selnum >= SELECT_NUM_MAX) {
		printf("select array is full\n");
		goto Err;
	}
	for (i = g_selnum - 1; i >= 0; i --) {
		if (g_selarray[i].fd == fd) {
			g_selnum --;
			for (; i < g_selnum; i ++)
				g_selarray[i] = g_selarray[i + 1];
			g_maxfd = -1;
			break;
		}
	}
Err:
	mid_mutex_unlock(g_mutex);
}
