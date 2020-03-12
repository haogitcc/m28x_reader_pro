#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "sqlite3.h"

#include "sqlite_m6e.h"
#include "server_m6e.h"
#include "mid_timer.h"

#define SQLITE_NAME "/opt/file/fuwit.db"

static sqlite3 *db = NULL;
int s_sqlite = 0;
static int count = 0;
static int ncount = 0;

int create_sqlite(void)
{
	char *zErrMsg = 0;
	int rc;

	rc = sqlite3_open(SQLITE_NAME, &db);
	//rc = sqlite3_open_v2(SQLITE_NAME, &db, SQLITE_OPEN_READWRITE, NULL);
	if(rc)
	{
		//fprintf(strerr,"can't open database:%s\n",sqlite_errmsg(db));
		printf("can't open database!\n");
		sqlite3_close(db);
		return -1;
	}

	char *sql = "CREATE TABLE fuwit(epc_id interger primary key,EPC VARCHAR(128));";
	sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
	printf("zErrMsg = %s\n",zErrMsg);
	return 0;
}

int sqlite_insert_v(int id,char *epcstr)
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


int sqlite_get_v()
{
	if(!db)
		return -1;
	int nrow = 0, ncolumn = 0, i = 0;
	char *zErrMsg = 0;
	char **sResult;
	int ret = -1;
	char sql[256];
	strcpy(sql,"SELECT * FROM fuwit;");
	sqlite3_get_table(db,sql,&sResult,&nrow,&ncolumn,&zErrMsg);
	if(nrow != 0) {
		count = atoi(sResult[(nrow + 1)*ncolumn -1]);
		ncount = count - atoi(sResult[2]);
	} else
		return 0;
	sqlite3_free_table(sResult);
	return 0;
}

int sqlite_send()
{
	const char *query = "SELECT * FROM fuwit;";
	sqlite3_stmt *stmt;
	char *zErrMsg = 0;
	char sql[256];
	
	int result = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
	if(SQLITE_OK == result) {
		while(SQLITE_ROW == sqlite3_step(stmt))
		{
			int id = sqlite3_column_int(stmt, 1);
			char *value = (char *)sqlite3_column_text(stmt, 2);
			if(0 == sqlite3_send(strlen(value), value))
			{
				sprintf(sql,"DELETE FROM fuwit WHERE epc_id=%d;",id);
				sqlite3_exec(db,sql,NULL,NULL,&zErrMsg);
				printf("delete zErrMsg = %s\n",zErrMsg);
				ncount--;
			} else {
				return -1;
			}
		}
		sqlite3_finalize(stmt);
	}
	count = 0;
	return 0;
}

int sqlite_close_ex()
{
	if(!db)
		return -1;
	sqlite3_close(db);
	return 0;
}

int sqlite_task()
{
	while(1) {
	//printf("sqlite task = %d!\n",s_sqlite);
		//if(ncount)
			//sqlite_get_t();
		sleep(5);
	}
}
