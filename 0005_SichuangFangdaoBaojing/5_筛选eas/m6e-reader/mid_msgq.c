
#include <unistd.h>
#include <time.h>

#include "mid_mutex.h"
#include "mid_msgq.h"
#include <sys/select.h>
#include <fcntl.h>

#define ERR_OUT() \
do{                                     \
    printf("error!\n");  \
    goto Err;                           \
} while(0)


struct mid_msgq {
	int size;
	int fds[2];
};

void ind_net_noblock(int fd)
{
#if defined(_WIN32)
#else
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
#endif
}

mid_msgq_t mid_msgq_create(int msg_num, int msg_size)
{
	mid_msgq_t msgq;

	msgq = (mid_msgq_t)malloc(sizeof(struct mid_msgq));
	if(msgq == NULL) {
		printf("malloc\n");
		ERR_OUT();
	}
	if(pipe(msgq->fds)) {
		free(msgq);
		printf("pipe\n");
		ERR_OUT();
	}
	ind_net_noblock(msgq->fds[0]);
	ind_net_noblock(msgq->fds[1]);
	msgq->size = msg_size;

	return msgq;
Err:
	return NULL;
}

int mid_msgq_delete( mid_msgq_t msgq)
{
	if(msgq == NULL)
		return -1;
	close(msgq->fds[0]);
	close(msgq->fds[1]);
	free(msgq);
	return 0;
}

int mid_msgq_fd(mid_msgq_t msgq)
{
	if(msgq == NULL) {
		printf("msgq = %p\n", msgq);
		ERR_OUT();
	}
	return msgq->fds[0];
Err:
	return -1;
}

int mid_msgq_put(mid_msgq_t msgq, char *msg, int sec)
{
	int ret;
	fd_set wset;
	struct timeval tv;

	if(msgq == NULL || sec < 0) {
		printf("msgq = %p, sec = %d\n", msgq, sec);
		ERR_OUT();
	}
	tv.tv_sec = sec;
	tv.tv_usec = 0;

	FD_ZERO(&wset);
	FD_SET( msgq->fds[1], &wset);

	ret = select(msgq->fds[1] + 1, NULL, &wset, NULL, &tv);
	if(ret <= 0) {
		printf("select ret = %d\n", ret);
		ERR_OUT();
	}
	ret = write(msgq->fds[1], msg, msgq->size);
	if (ret != msgq->size) {
		printf("ret = %d, msgq->size = %d\n", ret, msgq->size);
		ERR_OUT();
	}
	return 0;
Err:
	return -1;
}

int mid_msgq_get(mid_msgq_t msgq, char *msg, int sec, int usec)
{
	int ret;

	if(msgq == NULL || sec < 0 || usec < 0) {
		printf("msgq = %p, sec = %d, usec = %d\n", msgq, sec, usec);
		ERR_OUT();
        }
	if (sec > 0 || usec > 0) {
		fd_set rset;
		struct timeval tv;

		tv.tv_sec = sec;
		tv.tv_usec = usec;

		FD_ZERO(&rset);
		FD_SET( msgq->fds[0], &rset);

		ret = select(msgq->fds[0] + 1, &rset, NULL, NULL, &tv);
		if (ret == 0)
			return -1;
		if(ret <= 0) {
			printf("select ret = %d\n", ret);
			ERR_OUT();
		}
	}
	ret = read(msgq->fds[0], msg, msgq->size);
	if (ret != msgq->size) {
		printf("ret = %d, msgq->size = %d\n", ret, msgq->size);
		ERR_OUT();
	}
	return msgq->size;
Err:
	return -1;
}
int mid_msgq_putmsg(mid_msgq_t msgq, char *msg)
{
	int ret;

	if(msgq == NULL) {
		printf("msgq = %p\n", msgq);
		ERR_OUT();
	}
	ret = write(msgq->fds[1], msg, msgq->size);
	if (ret != msgq->size) {
		printf("ret = %d, msgq->size = %d\n", ret, msgq->size);
		ERR_OUT();
	}
	return 0;
Err:
	return -1;
}

int mid_msgq_getmsg(mid_msgq_t msgq, char *msg)
{
	int ret;

	if(msgq == NULL) {
		printf("msgq = %p\n", msgq);
		ERR_OUT();
	}
	ret = read(msgq->fds[0], msg, msgq->size);
	if (ret != msgq->size) {
		printf("ret = %d, msgq->size = %d\n", ret, msgq->size);
		ERR_OUT();
	}
	return msgq->size;
Err:
	return -1;
}

//#include "mid_include.h"

#include <unistd.h>
#include <pthread.h>

typedef struct int_elem* int_elem_t;
struct int_elem {
	int_elem_t next;
	int_elem_t prev;
	char* buf;
};

typedef struct mid_queue* mid_queue_t;

struct mid_queue {
	pthread_mutex_t mutex;
	int num;
	int size;
	int_elem_t pool;
	int_elem_t head;
	int_elem_t tail;
};

static pthread_mutex_t	g_mutex = PTHREAD_MUTEX_INITIALIZER;

mid_queue_t mid_queue_create(int msg_num, int msg_size)
{
	int i;
	int_elem_t elem;
	mid_queue_t queue;

	queue = (mid_queue_t)malloc(sizeof(struct mid_queue));
	if(queue == NULL) {
		printf("malloc\n");
		ERR_OUT();
	}
	memset(queue, 0, sizeof(struct mid_queue));

	pthread_mutex_init(&queue->mutex, NULL);

	queue->num = msg_num;
	queue->size = msg_size;

	for (i = 0; i < msg_num; i ++) {
		elem = (int_elem_t)malloc(sizeof(struct int_elem) + msg_size);
		if (elem) {
			elem->buf = (char*)elem + sizeof(struct int_elem);
			elem->next = queue->pool;
			queue->pool = elem;
		}
	}

	return queue;
Err:
	return NULL;
}

static void int_elem_free(int_elem_t elem)
{
	int_elem_t next;

	while(elem) {
		next = elem->next;
		free(elem);
		elem = next;
	}
}

void mid_queue_delete(mid_queue_t queue)
{
	if(queue == NULL) {
		printf("queue = %p\n", queue);
		ERR_OUT();
	}
	int_elem_free(queue->pool);
	queue->pool = NULL;

	int_elem_free(queue->head);
	queue->head = NULL;

	queue->tail = NULL;

Err:
	return;
}

int mid_queue_put(mid_queue_t queue, char *msg)
{
	int_elem_t elem;

	if (queue == NULL) {
		printf("msgq = %p\n", queue);
		ERR_OUT();
	}
	pthread_mutex_lock(&g_mutex);

	if (queue->pool) {
		elem = queue->pool;
		queue->pool = elem->next;

		elem->prev = NULL;
		if (queue->head)
			queue->head->prev = elem;
		else
			queue->tail = elem;

		elem->next = queue->head;
		queue->head = elem;

		memcpy(elem->buf, msg, queue->size);
	} else {
		printf("queue is full!\n");
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
Err:
	return -1;
}

int mid_queue_get(mid_queue_t queue, char *msg, int usec)
{
	int ret = -1;

	if (queue == NULL) {
		printf("msgq = %p\n", queue);
		ERR_OUT();
	}
	pthread_mutex_lock(&g_mutex);

	if (queue->head == NULL && usec > 0) {
		pthread_mutex_unlock(&g_mutex);
		usleep(usec);
		pthread_mutex_lock(&g_mutex);
	}

	if (queue->head) {
		int_elem_t prev, elem;

		elem = queue->tail;
		prev = elem->prev;
		if (prev)
			prev->next = NULL;
		else
			queue->head = NULL;

		queue->tail = prev;

		elem->next = queue->pool;
		queue->pool = elem;

		memcpy(msg, elem->buf, queue->size);

		ret = queue->size;
	} else {
		ret = 0;
	}

	pthread_mutex_unlock(&g_mutex);

Err:
	return ret;
}
