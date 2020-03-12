#ifndef __MID_MSGQ_H__
#define __MID_MSGQ_H__


#ifdef __cplusplus
extern "C" {
#endif
typedef struct mid_msgq* mid_msgq_t;

mid_msgq_t mid_msgq_create(int msg_num, int msg_size);
int mid_msgq_put(mid_msgq_t msgq, char *msg, int sec);
int mid_msgq_get(mid_msgq_t msgq, char *msg, int sec, int usec);
int mid_msgq_fd(mid_msgq_t msgq);
int mid_msgq_delete(mid_msgq_t msgq);
int mid_msgq_fd(mid_msgq_t msgq);
int mid_msgq_putmsg(mid_msgq_t msgq, char *msg);
int mid_msgq_getmsg(mid_msgq_t msgq, char *msg);

#ifdef __cplusplus
}
#endif

#endif /* __MID_MSGQ_H__ */


