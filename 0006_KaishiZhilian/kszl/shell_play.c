#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "shell.h"
#include "app_sys_setting.h"

/*
//mpcm /root/1.wav 8000 8 1
#define MIX_READ_SIZE    4096
static int shell_mix_pcm(int argc, char **argv)
{
    FILE *fp = NULL;
    int len;
    char buf[MIX_READ_SIZE];
    uint32_t magic;
    struct MixPCM mixpcm;

    mixpcm.sampleRate = atoi(argv[1]);
    mixpcm.bitWidth = atoi(argv[2]);
    mixpcm.channels = atoi(argv[3]);

    PRINTF("mid_stream_open\n");
    magic = mid_stream_open(0, (char*)&mixpcm, APP_TYPE_MIX_PCM, 0);
    mid_stream_sync(0, 1000);
    len = mid_stream_mix_space(0, magic);
    PRINTF("@ len = %d\n", len);

    fp = fopen(argv[0], "rb");
    if (fp == NULL)
        ERR_OUT("fopen! %s\n", argv[0]);

    fseek(fp, 0x3c, SEEK_SET);

    while(1) {
        for (;;) {
            len = mid_stream_mix_space(0, magic);
            if (len > 0)
                break;
            PRINTF("wait ...\n");
            mid_task_delay(100);
        }
        if (len > MIX_READ_SIZE)
            len = MIX_READ_SIZE;

        len = fread(buf, 1, len, fp);
        if (len < 0)
            ERR_OUT("fread!\n");
        if (len == 0) {
            mid_task_delay(45000);
            break;
        }

        mid_stream_mix_push(0, magic, buf, len);
    }

Err:
    if (fp)
        fclose(fp);
    PRINTF("mid_stream_close\n");
    mid_stream_close(0, 0);
    return 0;
}

*/
int shell_ipset(int argc, char *argv[])
{
	if(!(argv[0] && inet_addr(argv[0]) != INADDR_NONE))
		return -1;
	if(!(argv[1] && inet_addr(argv[1]) != INADDR_NONE))
		return -1;
	if(!(argv[2] && inet_addr(argv[2]) != INADDR_NONE))
		return -1;
	sysSettingSetString("ip", argv[0]);
	sysSettingSetString("netmask", argv[1]);
	sysSettingSetString("gateway", argv[2]);
	sys_config_save();
	return 0;
}

int set_serveripandport(int argc, char *argv[])
{
	if(!(argv[0] && inet_addr(argv[0]) != INADDR_NONE))
		return -1;
	if(atoi(argv[1]) == 0)
		return -1;
	sysSettingSetString("serverip", argv[0]);	
	sysSettingSetInt("serverport", atoi(argv[1]));
	sys_config_save();
	return 0;
	
}

int set_nettype(int argc, char *argv[])
{
	int net = atoi(argv[0]);
	if(net > 2)
		return -1;
	sysSettingSetInt("nettype", net);
	return 0;
}

int set_id(int argc, char *argv[])
{
	if(!argv[0])
		return -1;
	sysSettingSetString("id", argv[0]);	
	sys_config_save();
	return 0;
}

int set_wifi(int argc, char *argv[])
{	
	if(argc < 2)
		return -1;
	sysSettingSetString("wifi", argv[0]);
	sysSettingSetString("wifiwd", argv[1]);	
	return 0;
}


void shell_play_init(void)
{
  //  telnetd_regist("mpcm",      shell_mix_pcm,      "sddd");
	telnetd_regist("ipset",      shell_ipset,      "sss");  
    telnetd_regist("serverip",	   set_serveripandport, 	 "sd");
	telnetd_regist("nettype",	   set_nettype, 	 "d");
	telnetd_regist("device_id",	   set_id, 	 "s");
	//telnetd_regist("serverip",	   set_wifi, 	 "ss");
}
