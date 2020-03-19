#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>

#include "mid_mutex.h"
#include "ind_cfg.h"
#include "ind_string.h"
#include "mid_timer.h"
#include "m6e_upgrade.h"


#define CONFIG_LEN (32 * 1024)
#define NET_LEN 16
#define CONFIG_FILE_DIR "/opt/file"
#define NET_ETH 0
#define STRING_LEN_64 64



struct SYS_SETTINGS {
    int	nettype; // The network physical link type. 0 is cable, 1 is wifi.
    char ip[NET_LEN]; // static IP address
    char netmask[NET_LEN]; // static network submask
    char gateway[NET_LEN]; // static network gateway
    char dns[NET_LEN]; // static dns server address
    char dns1[NET_LEN];// static backup dns server address
    char serverip[NET_LEN];
	int serverport;
	char wifiname[STRING_LEN_64];
	char wifipassword[STRING_LEN_64];
	int region;
	int readpower;
	int writepower;
	int blf;
	int tari;
	int session;
	int targencoding;
	int target;
};

static struct SYS_SETTINGS sysConfigs;
static mid_mutex_t g_mutex = NULL;
static int gSaveMsgRecordCount = 0;
static CfgTree_t g_cfgTree = NULL;
static char *g_cfgBuf = NULL;
static int isTimer = 0;

void sys_cfg_inset_object(char *paramname)
{
    ind_cfg_inset_object(g_cfgTree, paramname);
}

void sys_cfg_inset_int(char *paramname, int *addr)
{
    ind_cfg_inset_int(g_cfgTree, paramname, addr);
}

void sys_cfg_set_visible(char *paramname, int visible)
{
    ind_cfg_set_visible(g_cfgTree, paramname, visible);
}

void sys_cfg_inset_string(char *paramname, char *addr, int len)
{
    ind_cfg_inset_string(g_cfgTree, paramname, addr, len);
}

/* 文件中增加校验数据 */
int sys_cfg_read(char *rootname)
{
    int len = 0;
    FILE *fp = NULL;
    int i_check_sum = 0;
    char c_check_sum[32 + 4];
    char filename[STRING_LEN_64];

    len = strlen(rootname);
    if(len > 16) {
        printf("%s too large\n", rootname);
        return -1;
    }
    sprintf(filename, CONFIG_FILE_DIR"/yx_config_%s.ini", rootname);
    if (access(filename, (F_OK | R_OK))) {
        printf("[%s] not exist or can not read\n", filename);
        return -1;
    }

    fp = fopen(filename, "rb");
    if(NULL == fp) {
        printf("Can not open file [%s]\n", filename);
        return -1;
    }

    len = fread(g_cfgBuf, 1, CONFIG_LEN, fp);
    fclose(fp);
    if(len <= 0 || len >= CONFIG_LEN) {
        printf("len = %d, CONFIG_LEN = %d\n", len, CONFIG_LEN);
        return -1;
    }
    g_cfgBuf[len] = 0;
    ind_cfg_input(g_cfgTree, rootname, g_cfgBuf, len);
    return 0;
}

int sys_cfg_write(char *rootname)
{
    int len = 0;
    FILE *fp = NULL;
    char filename[64] = {0};


    len = strlen(rootname);
    if(len > 16) {
        printf("%s too large\n", rootname);
        return -1;
    }

    if(mid_mutex_lock(g_mutex))
        printf("\n");

    sprintf(filename, CONFIG_FILE_DIR"/yx_config_%s.ini", rootname);
    len = ind_cfg_output(g_cfgTree, rootname, g_cfgBuf, CONFIG_LEN);
    if(len <= 0) {
        printf("tree_cfg_input = %d\n", len);
        mid_mutex_unlock(g_mutex);
        return -1;
    }

    mode_t oldMask = umask(0077);
    fp = fopen(filename, "wb");
    umask(oldMask);
    if(NULL == fp) {
        printf("fopen = %s\n", filename);
        mid_mutex_unlock(g_mutex);
        return -1;
    }

    fwrite(g_cfgBuf, 1, len, fp);
    fclose(fp);
    sync();

    mid_mutex_unlock(g_mutex);
    return 0;
}

int sys_config_init(void)
{
    if(g_cfgTree)
        return 0;

    g_mutex = mid_mutex_create();
    if(g_mutex == NULL) {
        printf("mid_mutex_create");
        return -1;
    }

    g_cfgBuf = (char *)malloc(CONFIG_LEN);
    if(g_cfgBuf == NULL) {
        printf("malloc :: memory");
        return -1;
    }

    g_cfgTree = ind_cfg_create();
    if(g_cfgTree == NULL) {
        printf("tree_cfg_create\n");
        if(g_cfgBuf)
            free(g_cfgBuf);
        return -1;
    }

    sys_cfg_inset_object("system");
    sys_cfg_inset_int("system.nettype", &sysConfigs.nettype);
    sys_cfg_inset_string("system.ip", sysConfigs.ip, NET_LEN);
    sys_cfg_inset_string("system.netmask", sysConfigs.netmask, NET_LEN);
    sys_cfg_inset_string("system.gateway", sysConfigs.gateway,	NET_LEN);
    sys_cfg_inset_string("system.dns", sysConfigs.dns,	NET_LEN);
    sys_cfg_inset_string("system.dns1", sysConfigs.dns1, NET_LEN);	
    sys_cfg_inset_string("system.serverip", sysConfigs.serverip,NET_LEN);	
    sys_cfg_inset_int("system.serverport", &sysConfigs.serverport);
	sys_cfg_inset_string("system.wifiname", &sysConfigs.wifiname,STRING_LEN_64);
	sys_cfg_inset_string("system.wifipassword", &sysConfigs.wifipassword,STRING_LEN_64);
	sys_cfg_inset_int("system.region", &sysConfigs.region);
	sys_cfg_inset_int("system.readpower", &sysConfigs.readpower);
	sys_cfg_inset_int("system.writepower", &sysConfigs.writepower);
	sys_cfg_inset_int("system.blf", &sysConfigs.blf);
	sys_cfg_inset_int("system.tari", &sysConfigs.tari);
	sys_cfg_inset_int("system.targencoding", &sysConfigs.targencoding);
	sys_cfg_inset_int("system.target", &sysConfigs.target);
	sys_cfg_inset_int("system.session", &sysConfigs.session);
    return 0;
}

void sys_config_load(int reset)
{
    if(mid_mutex_lock(g_mutex))
        printf("\n");

    memset(&sysConfigs, 0, sizeof(struct SYS_SETTINGS));


    sysConfigs.nettype = 1;
    strcpy(sysConfigs.ip, "192.168.8.166");
    strcpy(sysConfigs.netmask, "255.255.255.0");
    strcpy(sysConfigs.gateway, "192.168.8.1");
    strcpy(sysConfigs.dns, "10.10.10.2");
    strcpy(sysConfigs.dns1, "10.10.10.2");
    strcpy(sysConfigs.serverip, "192.168.0.250");
    sysConfigs.serverport= 8090;
	strcpy(sysConfigs.wifiname, "FUWIT");
	strcpy(sysConfigs.wifipassword, "fuwit1234");
	sysConfigs.region = 1;
	sysConfigs.readpower = 3000;
	sysConfigs.writepower = 3000;
	sysConfigs.blf = 1;
	sysConfigs.tari = 1;
	sysConfigs.session = 0;
	sysConfigs.targencoding = 1;
	sysConfigs.target = 0;

    mid_mutex_unlock(g_mutex);

    if(reset == 0) {
        if(0 == sys_cfg_read("system")) {
            printf("Read system config file OK!\n");
        } else {    
			sys_config_save();
            printf("ERROR:Read system config file failed.\n");
        }
    } else {
        sys_config_save();
    }
    return ;
}


int sys_config_save(void)/* sysConfigSave */
{
	isTimer = 0;
    sys_cfg_write("system");
    printf("stb change some config info ,and save it \n");
    return 0;
}

int sys_config_timer()
{
	if(!isTimer) {
		mid_timer_create(3,1,(mid_timer_f)sys_config_save,0);
		isTimer = 1;
	}
	return 0;
}


int sysSettingGetString(const char* name, char* value, int valueLen, int searchFlag)
{
    if(mid_mutex_lock(g_mutex)){
        printf("mutex lock error. Get doubleStack.\n");
        return -1;
    }

   if (!strcmp(name, "ip"))
        strncpy(value, sysConfigs.ip, valueLen);
	else if(!strcmp(name,"netmask"))
		strncpy(value, sysConfigs.netmask, valueLen);
	else if(!strcmp(name,"gateway"))
		strncpy(value, sysConfigs.gateway, valueLen);
	else if(!strcmp(name,"dns"))
		strncpy(value, sysConfigs.dns, valueLen);
	else if(!strcmp(name,"dns1"))
		strncpy(value, sysConfigs.dns1, valueLen);
	else if(!strcmp(name,"serverip"))
		strncpy(value, sysConfigs.serverip, valueLen);
	else if(!strcmp(name, "wifi"))
		strncpy(value, sysConfigs.wifiname, valueLen);
	else if(!strcmp(name, "wifiwd"))
		strncpy(value, sysConfigs.wifipassword, valueLen);
    else
        printf("undefined parameter %s\n", name);

    mid_mutex_unlock(g_mutex);
    printf("name:%s, value:%s\n", name, value);
    return 0;
}

int sysSettingGetInt(const char* name, int* value, int searchFlag)
{
    if(mid_mutex_lock(g_mutex)){
        printf("mutex lock error. Get doubleStack.\n");
        return -1;
    }

    if (!strcmp(name, "nettype"))
        *value = sysConfigs.nettype;
    else if (!strcmp(name, "serverport"))
        *value = sysConfigs.serverport;
	else if (!strcmp(name, "region"))
		*value = sysConfigs.region;
	else if (!strcmp(name, "readpower"))
		*value = sysConfigs.readpower;
	else if (!strcmp(name, "writepower"))
		*value = sysConfigs.writepower;
	else if (!strcmp(name, "blf"))
		*value = sysConfigs.blf;
	else if (!strcmp(name, "tari"))
		*value = sysConfigs.tari;
	else if (!strcmp(name, "session"))
		*value = sysConfigs.session;
	else if (!strcmp(name, "targencoding"))
		*value = sysConfigs.targencoding;
	else if (!strcmp(name, "target"))
		*value = sysConfigs.target;
    else
        printf("undefined parameter %s\n", name);
	
    mid_mutex_unlock(g_mutex);
    printf("name:%s, value:%d\n", name, *value);
    return 0;
}

int sysSettingSetString(const char* name, const char* value)
{
    printf("name:%s, value:%s\n", name, value);
    if(mid_mutex_lock(g_mutex)){
        printf("mutex lock error. Get doubleStack.\n");
        return -1;
    }


    if (!strcmp(name, "ip")) {
        if(strcmp(sysConfigs.ip, value)) {
            strncpy(sysConfigs.ip, value, NET_LEN);
        }
    } else if (!strcmp(name, "netmask")) {
        if(strcmp(sysConfigs.netmask, value)) {
            strncpy(sysConfigs.netmask, value, NET_LEN);
        }
    } else if (!strcmp(name, "gateway")) {
        if(strcmp(sysConfigs.gateway, value)) {
            strncpy(sysConfigs.gateway, value, NET_LEN);
        }
    } else if (!strcmp(name, "serverip")) {
        if(strcmp(sysConfigs.serverip, value)) {
            strncpy(sysConfigs.serverip, value, NET_LEN);
        }
	} else if (!strcmp(name, "dns1")) {
        if(strcmp(sysConfigs.dns1, value)) {
            strncpy(sysConfigs.dns1, value, NET_LEN);
        }
    } else if (!strcmp(name, "dns")) {
       if(strcmp(sysConfigs.dns, value)) {
           strncpy(sysConfigs.dns, value, NET_LEN);
       }
    } else if(strcmp(name, "wifi")) {
    	if(strcmp(sysConfigs.wifiname, value)) {
			strncpy(sysConfigs.wifiname, value, STRING_LEN_64);
		}
    } else if (strcmp(name, "wifiwd")){
    	if(strcmp(sysConfigs.wifipassword, value)) {
			strncpy(sysConfigs.wifipassword, value, STRING_LEN_64);
		}
	} else {
        printf("undefined parameter %s: %d\n", name, value);
        mid_mutex_unlock(g_mutex);
        return -1;
	}
    mid_mutex_unlock(g_mutex);
    return 0;
}

int sysSettingSetInt(const char* name, const int value)
{
    printf("name:%s, value:%d\n", name,value);
    if(mid_mutex_lock(g_mutex)){
        printf("mutex lock error. Get doubleStack.\n");
        return -1;
    }

    if (!strcmp(name, "nettype")) {
        if(sysConfigs.nettype!= value) {
            sysConfigs.nettype= value;
        }
    } else if (!strcmp(name, "serverport")) {
        if(sysConfigs.serverport!= value) 
			sysConfigs.serverport = value;
	} else if (!strcmp(name, "region")) {
		if(sysConfigs.region!= value) 
			sysConfigs.region = value;
	} else if (!strcmp(name, "readpower")) {
		if(sysConfigs.readpower!= value) 
			sysConfigs.readpower = value;
	} else if (!strcmp(name, "writepower")) {
		if(sysConfigs.writepower!= value) 
			sysConfigs.writepower = value;
	} else if (!strcmp(name, "blf")) {
		if(sysConfigs.blf!= value) 
			sysConfigs.blf = value;
	} else if (!strcmp(name, "tari")) {
		if(sysConfigs.tari!= value) 
			sysConfigs.tari = value;
	} else if (!strcmp(name, "session")) {
		if(sysConfigs.session!= value) 
			sysConfigs.session = value;
	} else if (!strcmp(name, "target")) {
		if(sysConfigs.target!= value) 
			sysConfigs.target = value;
	} else if (!strcmp(name, "targencoding")) {
		if(sysConfigs.targencoding!= value) 
			sysConfigs.targencoding = value;
    } else {
        printf("undefined parameter %s: %d\n", name, value);
        mid_mutex_unlock(g_mutex);
        return -1;
    }

    mid_mutex_unlock(g_mutex);
    return 0;
}

int sys_dns_set(const char *str, int no)
{
    printf("Dns(%d) to (%s)\n", no, str);

    if(no == 1)
        sysSettingSetString("dns1", str);
    else
        sysSettingSetString("dns", str);

    return 0;
}

int sys_dns_get(char *addr, int no)
{
    if(no == 1)
        sysSettingGetString("dns1", addr, NET_LEN, 0);
    else
        sysSettingGetString("dns", addr, NET_LEN, 0);

    if(inet_addr(addr) == INADDR_NONE) //WZW modified to fix pc-lint warning 650
        strcpy(addr, "0.0.0.0");
    return 0;
}




