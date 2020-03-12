#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>

#include "m6e_upgrade.h"

#define  UPGRADE_FILE "/opt/upgrade/upgrade_temp.bin"
#define  UPGRADE_FILE_V "/opt/upgrade/reader"


FILE *fp = NULL;
static int length = 0;


int m6e_upgrade_openfile()
{
	if(fp)
		return 0;
	fp = fopen(UPGRADE_FILE, "wb+");
	if(!fp) {
		printf("open file failed!\n");
		return -1;
	}
	length = 0;
	return 0;
}

int m6e_upgrade_resolve(char *buffer)
{
	int ret;
	//char *buf = (char *)malloc(255);
	//memcpy(buf , buffer + 12, 240);
	//printf("buf = %d\n",strlen(buf));
	ret = fwrite(buffer + 12, 1, buffer[1] - 9 , fp);
	length += ret;
	sync();
	//free(buf);
	return ret;
}

int m6e_upgrade_check()
{
	struct stat filestat;
    FILE* rFp = NULL;
    FILE* wFp = NULL;
	int len;

	if(!stat(UPGRADE_FILE,&filestat)) {
		char *buf = (char*)malloc(filestat.st_size + 1);
		rFp = fopen(UPGRADE_FILE, "rb");
		if((fread(buf, 1, filestat.st_size, rFp) == filestat.st_size)) {
			if(!strncmp(buf, "789654", 6)) {
				len = filestat.st_size - 6;
				wFp = fopen(UPGRADE_FILE_V, "wb");
				if(wFp) {
					if(fwrite(buf + 7, 1, len, wFp) == len)
						fflush(wFp);
					fclose(wFp);
				}
				
			} else 
				printf("check failed!\n");
		}
		free(buf);
		//fclose(rFp);
		
		system("rm /opt/upgrade/upgrade_temp.bin");
		return 0;
	}
	printf("file exit!\n");
	return -1;
	
}

int m6e_upgrade_reboot()
{
	printf("m6e_upgrade_reboot!\n");
	m6e_upgrade_close();
	system("mv /opt/upgrade/reader /opt/app/reader");
	//system("rm /opt/upgrade/upgrade_temp.bin");
	system("chmod u+x /opt/app/reader");
	system("reboot");
}

int m6e_upgrade_close()
{
	fclose(fp);
	sync();
	return 0;
}
