
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>

#include "mid_mutex.h"

struct ringbuf {
	int write_idx;
	int read_idx;
	int read_max_idx;

	int buf_size;
	int elem_size;

	mid_mutex_t mutex;

	char buf[4];
};

struct ringbuf* rng_buf_create(int buf_size, int elem_size)
{
	struct ringbuf *rng;

	if (buf_size <= 0 || elem_size <= 0 || buf_size <= elem_size)
		return NULL;
	rng = (struct ringbuf *)malloc(sizeof(struct ringbuf) + buf_size + elem_size);
	if (rng == NULL)
		return NULL;
	memset(rng, 0, sizeof(struct ringbuf));
	rng->buf_size = buf_size;
	rng->elem_size = elem_size;
	rng->mutex = mid_mutex_create( );

	return rng;
}

void rng_buf_delete(struct ringbuf* rng)
{
	if (rng)
		free(rng);
}

void rng_buf_reset(struct ringbuf* rng)
{
	if (rng == NULL)
		return;
	rng->write_idx = 0;
	rng->read_max_idx = 0;
	rng->read_idx = 0;
}

int rng_buf_len(struct ringbuf* rng)
{
	int len;

	if (rng == NULL)
		return 0;
	mid_mutex_lock(rng->mutex);
	if (rng->read_max_idx > rng->write_idx)
		len = rng->read_max_idx - rng->read_idx;
	else
		len = rng->write_idx - rng->read_idx;
	mid_mutex_unlock(rng->mutex);

	return len;
}

void rng_buf_write(struct ringbuf* rng, char *buf, int len)
{
	int l;

	if (rng == NULL)
		return;
	mid_mutex_lock(rng->mutex);

	if (rng->read_max_idx > rng->write_idx)
		l = rng->read_idx - rng->write_idx;
	else
		l = rng->elem_size;
	if (len >= l)
		len = l;
	if (len <= 0)
		goto End;

	memcpy(rng->buf + rng->write_idx, buf, len);
	rng->write_idx += len;
	if (rng->read_max_idx < rng->write_idx)
		rng->read_max_idx = rng->write_idx;
	if (rng->write_idx > rng->buf_size)
		rng->write_idx = 0;

End:
	mid_mutex_unlock(rng->mutex);
}

//telnetd ×¨ÓÃ
int rng_buf_enter(struct ringbuf* rng, char *buf, int len)
{
	int n, l;
	char ch, *p;

	if (rng == NULL)
		return -1;
	mid_mutex_lock(rng->mutex);

	if (rng->read_max_idx > rng->write_idx)
		l = rng->read_idx - rng->write_idx;
	else
		l = rng->elem_size;

	//PRINTF("l = %d, len = %d\n", l, len);
	n = 0;
	p = rng->buf + rng->write_idx;
	while(len > 0) {
		if (l - n < 2)
			break;
		ch = buf[0];
		switch(ch) {
		case '\r':
			break;
		case '\n':
			*p ++ = '\r';
			*p ++ = '\n';
			n += 2;
			break;
		default:
			*p ++ = ch;
			n ++;
			break;
		}
		len --;
		buf ++;
	}
	rng->write_idx += n;
	if (rng->read_max_idx < rng->write_idx)
		rng->read_max_idx = rng->write_idx;
	if (rng->write_idx > rng->buf_size)
		rng->write_idx = 0;

	//PRINTF("n = %d, write_idx = %d, read_max_idx = %d\n", n, rng->write_idx, rng->read_max_idx);

	mid_mutex_unlock(rng->mutex);

	return n;
}

int rng_buf_read(struct ringbuf* rng, char *buf, int size)
{
	int len;

	if (rng == NULL)
		return 0;
	mid_mutex_lock(rng->mutex);

	if (rng->read_max_idx > rng->write_idx)
		len = rng->read_max_idx - rng->read_idx;
	else
		len = rng->write_idx - rng->read_idx;
	if (len > size)
		len = size;
	if (len <= 0)
		goto End;

	memcpy(buf, rng->buf + rng->read_idx, len);
	rng->read_idx += len;
	if (rng->read_max_idx > rng->write_idx && rng->read_max_idx == rng->read_idx) {
		rng->read_max_idx = rng->write_idx;
		rng->read_idx = 0;
	}

End:
	mid_mutex_unlock(rng->mutex);

	return len;
}

int rng_buf_send(struct ringbuf* rng, int fd)
{
	int len;

	if (rng == NULL || fd < 0)
		return 0;
	mid_mutex_lock(rng->mutex);

	if (rng->read_max_idx > rng->write_idx)
		len = rng->read_max_idx - rng->read_idx;
	else
		len = rng->write_idx - rng->read_idx;
	if (len <= 0)
		goto End;

	len = send(fd, rng->buf + rng->read_idx, len, MSG_NOSIGNAL);
	if (len <= 0)
		goto End;
	rng->read_idx += len;
	if (rng->read_max_idx > rng->write_idx && rng->read_max_idx <= rng->read_idx) {
		rng->read_max_idx = rng->write_idx;
		rng->read_idx = 0;
	}

End:
	mid_mutex_unlock(rng->mutex);

	return len;
}

