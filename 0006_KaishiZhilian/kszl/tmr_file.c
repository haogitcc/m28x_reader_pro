#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>

#include "tmr_file.h"
#include "mid_mutex.h"
#define CONFIG_FILE_DIR "/opt/file"

static mid_mutex_t g_mutex = NULL;

int tmr_file_init()
{
    g_mutex = mid_mutex_create();
    if(g_mutex == NULL) {
        printf("mid_mutex_create");
        return -1;
    }	
}

int tmr_file_read(int fd)
{
    int ret = 0;
    FILE *fp = NULL;
    char filename[64] = {0};

    if(mid_mutex_lock(g_mutex))
        printf("\n");

    sprintf(filename, CONFIG_FILE_DIR"/tmr_tag_%s.txt", "data");

    mode_t oldMask = umask(0077);
    fp = fopen(filename, "rt");
    umask(oldMask);
    if(NULL == fp) {
        printf("fopen = %s\n", filename);
        mid_mutex_unlock(g_mutex);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    printf("len : %d\n", len);
    char *str = (char*)malloc(len + 1);
    memset(str, 0, len+1);
    fseek(fp, 0, SEEK_SET);
    ret = fread(str, 1, len, fp);
    if(ret <= 0 || ret > len) {
        printf("len = %d, CONFIG_LEN = %d\n", ret, len);
	 free(str);
	 goto END;
    }
	
    int tagcount = 0;
    sysSettingGetInt("tagcount", &tagcount,1);
	if(mid_tagdata_server(str, tagcount, fd) < 0)
	{
		printf("send filed!\n");
		free(str);
		goto END;
	} else {
		if(socket_receive_ex(5000, fd) <= 0) {
			printf("send filed!\n");
			free(str);
			goto END;
		}
		 //fseek(fp, 0, SEEK_SET);
		 //fwrite("", 1, 1, fp);
		 printf("***********************8\n");
		 //ftruncate(fp, 0);
		 fclose(fp);
		 free(str);

		 fp = fopen(filename, "w");
		 fclose(fp);
		 
   		 mid_mutex_unlock(g_mutex);
    		 return 0;
	}

END:
    fclose(fp);
    mid_mutex_unlock(g_mutex);
    return -1;
}

int tmr_file_write(char *str, int number)
{
    int len = 0;
    FILE *fp = NULL;
    char filename[64] = {0};
    char value[64] = {0};
	
    if(mid_mutex_lock(g_mutex))
        printf("\n");

    sprintf(filename, CONFIG_FILE_DIR"/tmr_tag_%s.txt", "data");

    mode_t oldMask = umask(0077);
    fp = fopen(filename, "a");
    umask(oldMask);
    if(NULL == fp) {
        printf("fopen = %s\n", filename);
        mid_mutex_unlock(g_mutex);
        return -1;
    }
    lseek(fp, 0, SEEK_END);
    sprintf(value, "%s,%d;", str, number);
    printf("str: %s,  %d\n", value, strlen(value));
    fwrite(value, 1, strlen(value), fp);
    fclose(fp);
    sync();
    mid_mutex_unlock(g_mutex);
    return 0;
}

