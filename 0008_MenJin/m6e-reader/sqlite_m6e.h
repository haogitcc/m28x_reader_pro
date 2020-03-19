#ifndef _SQLITE_M6E_H
#define _SQLITE_M6E_H

int create_sqlite(void);
int sqlite_insert(int id,char *epcstr);
int sqlite_get();
int sqlite_close_ex();
int sqlite_task();
int sqlite_get_v();



#endif

