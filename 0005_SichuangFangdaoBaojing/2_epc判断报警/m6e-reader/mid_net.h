#ifndef _MID_NET_H
#define _MID_NET_H

//int osex_netlink(void);
void osex_net_close( );
int osex_gateway_get(const char *ifname, char *buf);
int osex_ipaddr_get(const char* ifname, char* buf);
int osex_ipaddr_set(const char* ifname, char* ip);
int osex_netmac_get(const char *ifname, char *mac);
int osex_ipmask_get(char *ifname, char *buf);
int osex_ipmask_set(const char* ifname, char* ip);
int osex_netmac_get(const char *ifname, char *mac);
int osex_iproute_add(const char* dst, const char* mask, const char* gw, int metric);
int mid_connect();


#endif


