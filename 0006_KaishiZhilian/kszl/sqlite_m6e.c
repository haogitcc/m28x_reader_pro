#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <linux/tcp.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/time.h>
#include <linux/rtc.h>
#include "sqlite3.h"

#include "sqlite_m6e.h"
#include "server_m6e.h"
#include "mid_timer.h"
#include "client_data.h"
#include "mid_mutex.h"

#define SQLITE_DATA  "/opt/sqlite/fuwit.db"

static sqlite3 *db = NULL;
int s_sqlite = 0;
static int count_n = 0;

typedef struct tagSend {
	tagEPC *tagQueue;
}tagSend;



sem_t tag_length;
tagSend *head = NULL;


static mid_mutex_t mutex = NULL;

int mid_sqlite_send(char *str, int count, int fd)
{
	if(mid_tagdata_server(str, count, fd) < 0)
	{
		printf("send filed!\n");
		return -1;
	} else {
		printf("socket_receive_ex\n");
		int ret = socket_receive_ex(3000, fd);
		return ret;
	}

}
int pthread_tag_init()
{
      mutex = mid_mutex_create();
      if(mutex == NULL) {
         printf("mid_mutex_create");
         return -1;
      }

      sem_init(&tag_length, 0, 0);
	  
	head = (tagSend *)malloc(sizeof(tagSend));
	 return 0;	  
}

tagEPC * 
dequeue_p()
{
  tagEPC *tagRead;
  pthread_mutex_lock(mutex);
  tagRead = head->tagQueue;
  head->tagQueue= head->tagQueue->next;
  pthread_mutex_unlock(mutex);
  return tagRead;
}

void enqueue_p(char *epcStr, char *time)
{
  pthread_mutex_lock(mutex);
  tagEPC *tagStr = (tagEPC *)malloc(sizeof(tagEPC));
  strcpy(tagStr->epc,epcStr);
  strcpy(tagStr->time,time);
  tagStr->next = head->tagQueue;
  head->tagQueue = tagStr;
  pthread_mutex_unlock(mutex);
  sem_post(&tag_length);
}

int create_sqlite(void)
{
	char *zErrMsg = 0;
	int rc;

	rc = sqlite3_open(SQLITE_DATA, &db);
	//rc = sqlite3_open_v2(SQLITE_NAME, &db, SQLITE_OPEN_READWRITE, NULL);
	if(rc)
	{
		//fprintf(strerr,"can't open database:%s\n",sqlite_errmsg(db));
		printf("can't open database!\n");
		sqlite3_close(db);
		return -1;
	}

	char *sql = "CREATE TABLE fuwit_data(epc_id integer primary key autoincrement,Number INTEGER, EPC VARCHAR(48), Time VARCHAR(32));";
	sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
	printf("zErrMsg = %s\n",zErrMsg);

	char *sql1 = "CREATE TABLE fuwit_number(id integer primary key autoincrement,Number INTEGER);";
	sqlite3_exec(db, sql1, NULL, NULL, &zErrMsg);
	printf("zErrMsg = %s\n",zErrMsg);
	return 0;
}

int create_sqlite_number(int fd, int num)
{
	char *zErrMsg = 0;
	int rc , ret = -1;
	char **sResult;
	int nrow = 0, ncolumn = 0, i = 0;
	const char *query = "SELECT * FROM fuwit_number;";
	sqlite3_stmt *stmt;
	char sql[256];
	memset(sql,'\0',256);
	int result = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);

	if(SQLITE_OK == result) {
		while(SQLITE_ROW == sqlite3_step(stmt))
		{
			int number = sqlite3_column_int(stmt, 1);
			//int number = sqlite3_column_int(stmt, 2);
			printf("create_sqlite_number :  %d, %d\n", number, num);
			if(number == num)
				continue;
			ret = sqlite_send(number, fd);
			if(ret < 0) {
				sqlite3_finalize(stmt);
				return -1;
			}
			
			sprintf(sql,"DELETE FROM fuwit_number WHERE Number=%d;",number);
			sqlite3_exec(db,sql,NULL,NULL,&zErrMsg);
			//printf("create_sqlite_number delete zErrMsg = %s\n",zErrMsg);	
			module_man_set(0);
		}
		sqlite3_finalize(stmt);
	}
	return 0;
}

int sqlite_insert_v()
{
	if(!db)
		return -1;
	char *zErrMsg = 0;
	tagEPC *tag = NULL;
	int i = 0, man = 0, number = 0;
	char sql[256];
	memset(sql,'\0',256);

	module_man_get(&man);
	if(man) {
		printf("sqlite over!\n");
		return -1;
	}

	sqlite3_exec(db, "begin;", 0, 0, &zErrMsg);
	while(1) {
		tag = dequeue_p();
	      if(tag) {
			module_number_get(&number);
			memset(sql,'\0',256);
			sprintf(sql,"INSERT INTO fuwit_data VALUES(NULL,%d,'%s','%s');", number, tag->epc, tag->time);
			sqlite3_exec(db,sql,NULL,NULL,&zErrMsg);
			//printf("exec zErrMsg = %s\n",zErrMsg);
			free(tag);
			tag = NULL;
		} else {
			break;
		}
	}

	sqlite3_exec(db, "commit;", 0, 0, &zErrMsg);
	printf("exec zErrMsg commit = %s\n",zErrMsg);
	mid_timer_create(2 , 1, (mid_timer_f)sqlite_insert_v, 0);
}

/************************************************ 
设置操作系统时间 
参数:*dt数据格式为"2006-4-20 20:30:30" 
调用方法: 
    char *pt="2006-4-20 20:30:30"; 
    SetSystemTime(pt); 
**************************************************/  
int SetSystemTime(char *dt)  
{  
    struct rtc_time tm;  
    struct tm _tm;  
    struct timeval tv;  
    time_t timep;  
    int ret = sscanf(dt, "%d-%d-%d %d:%d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday,&tm.tm_hour, &tm.tm_min, &tm.tm_sec);  
	if(ret <= 0) {
		printf("parse SetSystemTime error!\n");
		return -1;
	}
    _tm.tm_sec = tm.tm_sec;  
    _tm.tm_min = tm.tm_min;  
    _tm.tm_hour = tm.tm_hour;  
    _tm.tm_mday = tm.tm_mday;  
    _tm.tm_mon = tm.tm_mon - 1;  
    _tm.tm_year = tm.tm_year - 1900;  
  
    timep = mktime(&_tm);  
    tv.tv_sec = timep;  
    tv.tv_usec = 0;  
    if(settimeofday (&tv, (struct timezone *) 0) < 0)  
    {  
    	 printf("Set system datatime error!/n");  
   	 return -1;  
    }  
    return 0;  
} 

void sys_get_time(char *timestr)
{
	struct tm *t;
	time_t tt;
	time(&tt);
	t = localtime(&tt);
	sprintf(timestr, "%d-%02d-%02d-%02d-%02d-%02d", t->tm_year+1900, t->tm_mon +1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	return;
}

int sqlite_insert_data(char *epc ,char *time)
{
	if(!db)
		return -1;
	char *zErrMsg = 0;
	char sql[256];
	int number, man;
	module_man_get(&man);
	if(man) {
		printf("sqlite over!\n");
		return -1;
	}
		
	//sysSettingGetInt("number", &number, 1);
	module_number_get(&number);
	memset(sql,'\0',256);
	sprintf(sql,"INSERT INTO fuwit_data VALUES(NULL,%d,'%s','%s');", number, epc,time);
	//printf("sql: %s\n", sql);
	sqlite3_exec(db,sql,NULL,NULL,&zErrMsg);
	//printf("sqlite_insert_data exec zErrMsg = %s\n",zErrMsg);
}

int sqlite_insert_number(int number)
{
	if(!db)
		return -1;
	if(sqlite_get_total() > 10000) {
		printf("row 10000\n");
		module_man_set(1);
		return -1;
	}
	char *zErrMsg = 0;
	char sql[256];
	memset(sql,'\0',256);
	sprintf(sql,"INSERT INTO fuwit_number VALUES(NULL,%d);", number);
	sqlite3_exec(db,sql,NULL,NULL,&zErrMsg);
	printf("sqlite_insert_number exec zErrMsg = %s\n",zErrMsg);
}

/*int sqlite_insert_v(int id,char *epcstr)
{
	if(!db)
		return -1;
	char *zErrMsg = 0;
	tagEPC *tag;
	int i = 0;
	char sql[256];
	memset(sql,'\0',256);

	sqlite3_exec(db, "begin;", 0, 0, &zErrMsg);
conutry:
	tag = dequeue_p();
	if(tag) {
		count++;
		i++;
		sprintf(sql,"INSERT INTO fuwit VALUES(%d,'%s');",count,tag->epc);
		sqlite3_exec(db,sql,NULL,NULL,&zErrMsg);
		printf("exec zErrMsg = %s\n",zErrMsg);
		if(!zErrMsg) {
			ncount++;
			if(i < 500)
				goto conutry;
		}
	}

	sqlite3_exec(db, "commit;", 0, 0, &zErrMsg);
	mid_timer_create(1 , 1, (mid_timer_f)sqlite_insert_v, 0);

}
*/

int sqlite_get_total()
{
	char sql[256];
	int nrow = 0, ncolumn = 0, i = 0;
	strcpy(sql,"SELECT * FROM fuwit_data;");
	char *zErrMsg = 0;
	char **sResult;
	
	sqlite3_get_table(db,sql,&sResult,&nrow,&ncolumn,&zErrMsg);
	sqlite3_free_table(sResult);
	return nrow;
}

int sqlite_get_count()
{
	if(!db)
		return -1;
	int nrow = 0, ncolumn = 0, i = 0;
	char *zErrMsg = 0;
	char **sResult;
	int ret = -1;
	char sql[256];
	strcpy(sql,"SELECT * FROM fuwit_number;");
	sqlite3_get_table(db,sql,&sResult,&nrow,&ncolumn,&zErrMsg);
	if(nrow != 0) {
		count_n = atoi(sResult[(nrow + 1)*ncolumn -1]);
		printf("sqlite_get_count : %d\n", count_n);
		module_number_set(count_n +1);
	} else {
		module_number_set(2);
	}
	sqlite3_free_table(sResult);

	return 0;
}

int sqlite_send(int number, int fd)
{
	printf("sqlite_send: %d\n",number);
	sqlite3_stmt *stmt;
	char *zErrMsg = 0;
	char sql[256];
	char str[256];
	int count = 0, ret = -1;
	char *epcstr = (char *)malloc(5120);
	memset(epcstr, 0 ,5120);
	sprintf(sql,"SELECT * FROM fuwit_data WHERE Number=%d;",number);
	int result = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if(SQLITE_OK == result) {
		while(SQLITE_ROW == sqlite3_step(stmt))
		{
			count++;
			memset(str, 0 ,256);
			//int id = sqlite3_column_int(stmt, 1);
			int number = sqlite3_column_int(stmt, 1);
			char *epc = (char *)sqlite3_column_text(stmt, 2);
			char *time = (char *)sqlite3_column_text(stmt, 3);
			//printf("sqlite_send : %s, %s, %d\n",  epc,time, number);
			//printf("%s, %d\n", str, strlen(str));
			sprintf(str, "%s,%s,%d,%d;", epc, time, count, 1);
			epcstr = strcat(epcstr, str);
			//printf("epcstr : %d\n", strlen(epcstr));
			//strcpy(epcstr, str);
			//epcstr += strlen(str);
			if(count >= 60) {  //每次只发送100张标签
				ret = mid_sqlite_send(epcstr, count, fd);
				if(ret <= 0) {
					sqlite3_finalize(stmt);
					free(epcstr);
					return -1;
				} else {
					memset(epcstr, 0 ,5120);
					count = 0;
				}
			}
		}
		if(count > 0) {
			printf("epcstr : %s\n", epcstr);
			ret = mid_sqlite_send(epcstr, count, fd);
			if(ret <= 0) {
				sqlite3_finalize(stmt);
				free(epcstr); //????死机
				return -1;
			}
		}
		
		sprintf(sql,"DELETE FROM fuwit_data WHERE Number=%d;",number);
		sqlite3_exec(db,sql,NULL,NULL,&zErrMsg);
		printf("sqlite_send delete zErrMsg = %s\n",zErrMsg);
		sqlite3_finalize(stmt);
	}
	free(epcstr);
	return 0;
}

int sqlite_close_ex()
{
	if(!db)
		return -1;
	sqlite3_close(db);
	return 0;
}

