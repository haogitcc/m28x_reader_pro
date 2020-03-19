
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <arpa/inet.h>

#include <linux/if.h>
#include <linux/route.h>

//#include "yxapi.h"

#define SIOCGLINKSTATE      (SIOCDEVPRIVATE + 0)
static char g_interface[16] = "eth0"; //eth0,rausb0,ppp,eth0.xxxx,rausb0.xxxx
#ifndef IFNAMSIZ
#define IFNAMSIZ    16
#endif



/* sdk的检测网络在nfs挂接的时候好像存在问题 */
int osex_netlink(void)
{
    struct ifreq ifr;
	int so;
	int tLink = 0;

	so = socket (PF_PACKET, SOCK_RAW, 0);
	if (so < 0)
		return 0;

	strcpy(ifr.ifr_name,"eth0");
	ifr.ifr_data = (void *)&tLink;

	if (ioctl(so, SIOCGLINKSTATE, &ifr) == -1)
            printf("ioctl err\n");
	close(so);

    return tLink;
}

/*
 * OS function
 */
static int g_sockfd = -1;
void osex_net_close( )
{
	close(g_sockfd);
	g_sockfd = -1;
}

static inline int get_raw_socket( )
{
	int sockfd;

	if (g_sockfd > 0)
		return g_sockfd;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		goto Err;

	g_sockfd = sockfd;
	return sockfd;
Err:
	return -1;
}

static inline int raw_socket_req_r(const char* ifname, struct ifreq* pifr, int req)
{
	int ret;

	if (!ifname || !pifr) {
		printf("ifname = %p, pifr = %p\n", ifname, pifr);
		goto Err;
	}

	strncpy(pifr->ifr_name, ifname, IFNAMSIZ);

	ret = ioctl(get_raw_socket( ), req, pifr);
	if (ret < 0) {
		printf("ioctl ret = %d / %s\n", ret, strerror(errno));
		goto Err;
	}
	return 0;
Err:
	return -1;
}

static inline int raw_socket_req(const char* ifname, char* buf, int req)
{
	int ret;
	struct ifreq ifr;
	struct sockaddr_in* p;

	memset(&ifr, 0, sizeof(struct ifreq));
	ifr.ifr_addr.sa_family = AF_INET;
	ret = raw_socket_req_r(ifname, &ifr, req);
	if (ret < 0) {
		printf("raw_socket_req_r ret = %d\n", ret);
		goto Err;
	}
	p =(struct sockaddr_in*)&(ifr.ifr_addr);
	strncpy(buf,(char*)inet_ntoa(p->sin_addr), 16);

	return 0;
Err:
	return -1;
}

int osex_gateway_get(const char *ifname, char *buf)
{
    return raw_socket_req(ifname, buf,  SIOCGIFDSTADDR);
}

int osex_ipaddr_get(const char* ifname, char* buf)
{
	return raw_socket_req(ifname, buf, SIOCGIFADDR);
}

int osex_ipaddr_set(const char* ifname, char* ip)
{
	struct ifreq ifr;
	struct sockaddr_in* p;

	if (ip == NULL)
		goto Err;

	if (raw_socket_req_r(ifname, &ifr, SIOCGIFADDR) < 0)
		printf("raw_socket_req_r 1\n");

	p =(struct sockaddr_in*)&(ifr.ifr_addr);
	p->sin_addr.s_addr = inet_addr(ip);
	p->sin_port = 0;
	p->sin_family = AF_INET;

	if (raw_socket_req_r(ifname, &ifr, SIOCSIFADDR) < 0) {
		printf("raw_socket_req_r 2\n");
		goto Err;
	}
	if(raw_socket_req_r(ifname, &ifr, SIOCGIFFLAGS) < 0) {
		printf("raw_socket_req_r 3\n");
		goto Err;
	}
	if (!(ifr.ifr_flags & IFF_UP)){
		ifr.ifr_flags |= IFF_UP;
		if(raw_socket_req_r(ifname, &ifr, SIOCSIFFLAGS) <0)
			printf("raw_socket_req_r 4\n");
			goto Err;
	}

	return 0;
Err:
	return -1;
}
#define LINUX_PATH_RESOLV "/etc/resolv.conf"
#define LINUX_CONF_VAL      "/proc/sys/net/ipv4/conf/all/force_igmp_version"
int osex_ipdns_get(char *dns0, char *dns1)
{
	FILE *fp;
	int len;
	char buf[128];

	if (dns0 == NULL)
		goto Err;
	sprintf(dns0, "0.0.0.0");
	if (dns1)
		sprintf(dns1, "0.0.0.0");

	fp = fopen("/etc/resolv.conf", "r");
	if (fp == NULL)
		goto Err;
	len = fread(buf, 1, 128, fp);
	fclose(fp);

	if (len <= 0 || len >= 128)
		goto Err;

	if (dns1)
		sscanf(buf, "nameserver %s\nnameserver %s\n", dns0, dns1);
	else
		sscanf(buf, "nameserver %s\n", dns0);

	printf("@@@@@@@@: dns0 = %s\n", dns0);
	if (dns1)
		printf("@@@@@@@@: dns1 = %s\n", dns1);

	return 0;
Err:
	return -1;

}
int osex_ipdns_set(char *dns0, char *dns1)
{
    int fd = -1;
	int len;
	char buf[128];

	if (dns0 == NULL || strncmp(dns0, "0.0.0.0", 7) == 0 || strncmp(dns0, "255.255.255.255", 15) == 0)
		goto Err;

	len = sprintf(buf, "nameserver %s\n", dns0);
	if (dns1 && strncmp(dns1, "0.0.0.0", 7) != 0 && strncmp(dns1, "255.255.255.255", 15) != 0)
		len += sprintf(buf + len, "nameserver %s\n", dns1);

    fd = open( LINUX_PATH_RESOLV, O_CREAT | O_WRONLY );
    if( fd < 0 ){
        goto Err;
    }
    len = write( fd, buf, len+1 );
    if( len <= 0 ){
        close( fd );
        goto Err;
    }
    close( fd );
	return 0;
Err:
	return -1;
}
int osex_ipmask_get(char *ifname, char *buf)
{
	return raw_socket_req(ifname, buf, SIOCGIFNETMASK);
}

int osex_ipmask_set(const char* ifname, char* ip)
{
	struct ifreq ifr;
	struct sockaddr_in* p = 0;

	if (!ip)
		goto Err;

	if (raw_socket_req_r(ifname, &ifr, SIOCGIFNETMASK) < 0) {
		printf("request info failed!\n");
		goto Err;
	}
	p =(struct sockaddr_in*)&(ifr.ifr_addr);
	p->sin_addr.s_addr = inet_addr(ip);

	if (raw_socket_req_r(ifname, &ifr, SIOCSIFNETMASK) < 0){
		printf("request info failed!");
		goto Err;
	}
	return 0;
Err:
	return -1;
}

int osex_netmac_get(const char *ifname, char *mac)
{
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(struct ifreq));
	if(raw_socket_req_r(ifname, &ifr, SIOCGIFHWADDR) < 0) {
		printf("request info failed!\n");
		goto Err;
	}
	memcpy(mac,(char*)ifr.ifr_hwaddr.sa_data, 6);
	return 0;
Err:
	return -1;
}

int osex_iproute_add(const char* dst, const char* mask, const char* gw, int metric)
{
	int err;
	struct rtentry rt;
	struct sockaddr_in	*addr;

	if (!dst || !mask || !gw) {
		printf("dst = %p, mask = %p, gw = %p\n", dst, mask, gw);
		goto Err;
	}

	memset(&rt, 0 , sizeof(struct rtentry));

	addr =(struct sockaddr_in *)&rt.rt_dst;
	addr->sin_family = AF_INET;
	addr->sin_port = htons(0);
	addr->sin_addr.s_addr = inet_addr(dst);

	addr =(struct sockaddr_in *)&rt.rt_genmask;
	addr->sin_family = AF_INET;
	addr->sin_port = htons(0);
	addr->sin_addr.s_addr = inet_addr(mask);

	addr =(struct sockaddr_in *)&rt.rt_gateway;
	addr->sin_family = AF_INET;
	addr->sin_port = htons(0);
	addr->sin_addr.s_addr = inet_addr(gw);

	rt.rt_flags = RTF_GATEWAY;
	err = ioctl(get_raw_socket( ), SIOCADDRT, &rt);

	if (err && errno != EEXIST) {
		printf("ioctl err = %d, errno = %d\n", err, errno);
		goto Err;
	}
	return 0;
Err:
	return -1;
}

int osex_igmp_version_get(void)                              // add by Li  Sensen.
{
	int fd, ret, igver;

	if ((fd = open(LINUX_CONF_VAL, O_RDONLY)) == -1) {
		printf("GET IGMP VERSION ERROR! Can't open the config file.\n");
		goto Err;
	}
	ret = read(fd, &igver, 4);
	if (ret <= 0 || ret != 4) {
		close(fd);
		printf("read igver error!\n");
		goto Err;
	}

	close(fd);
	return igver;

Err:
	return -1;
}

int osex_igmp_version_set(const char *file, const char *igver)
{
	FILE *fp = NULL;
	int ret;

	if(igver == NULL || !isdigit(igver[0]))
		printf("The igver ptr error!\n");

	if(atoi(igver) != 2 && atoi(igver) != 3)
		printf("The value of igmp version error!\n");

	if(file != NULL && isprint(file[0])) {
		if((fp = fopen(file, "wb")) == NULL) {
			printf("SET IGMP VERSION ERROR! Open LINUX_CONF_VAL error!\n");
			goto Err;
		}
	} else {
		if((fp = fopen(LINUX_CONF_VAL, "wb")) == NULL) {
			printf("SET IGMP VERSION ERROR! Open LINUX_CONF_VAL error!\n");
			goto Err;
		}
	}

	ret = fwrite(igver, 1, 1, fp);

	if(ret <= 0) {
		fclose(fp);
		printf("write igver error!\n");
		goto Err;
	}

	fclose(fp);
	return 0;
Err:
	return -1;
}

/*int mid_net4_staticip_connect(void)
{
    char ipaddr[16] = {0};
    char netmask[16] = {0};
    char gateway[16] = {0};
    char dns0[16] = {0};
    char dns1[16] = {0};

    sysSettingGetString("ip", ipaddr, 16, 0);
    sysSettingGetString("netmask", netmask, 16, 0);
    sysSettingGetString("gateway", gateway, 16, 0);
    //sys_dns_get(dns0, 0);
    //sys_dns_get(dns1, 1);

    //mid_net_initval(ipaddr, netmask, gateway, dns0, dns1);

    osex_ipaddr_set(g_interface, ipaddr);
    osex_ipmask_set(g_interface, netmask);
    if(memcmp(gateway, "0.0.0.0", 7))
        osex_iproute_add("0.0.0.0", "0.0.0.0", gateway, 5);
    return 0;
}
*/

