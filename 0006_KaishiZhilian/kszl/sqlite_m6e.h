#ifndef _SQLITE_M6E_H
#define _SQLITE_M6E_H

int create_sqlite(void);
int sqlite_get();
int sqlite_close_ex();
int sqlite_get_v();
int mid_sqlite_send(char *str, int count, int fd);
int create_sqlite_number(int fd, int num);
int sqlite_send(int number, int fd);
int sqlite_insert_data(char *epc ,char *time);
int sqlite_insert_number(int number);
int sqlite_get_count();
int SetSystemTime(char *dt);
void enqueue_p(char *epcStr, char *time);
int pthread_tag_init();
int sqlite_insert_v();


typedef struct tagEPC {
	char epc[256];
	char time[48];
	struct tagEPC *next;
}tagEPC;

#endif

