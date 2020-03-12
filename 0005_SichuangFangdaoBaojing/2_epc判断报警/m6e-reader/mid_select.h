
#ifndef __MID_SELECT_H__
#define __MID_SELECT_H__

enum {
	SELECT_MSG_ERROR = -1,
	SELECT_MSG_READ,
	SELECT_MSG_WRITE
};

#ifndef NULL
#define NULL             0L
#endif


typedef void (*mid_select_f)(int arg);

//timeout 以秒为单位
int mid_select_regist(int fd, mid_select_f callback, int arg);

void mid_select_unregist(int fd);

#endif//__MID_SELECT_H__

