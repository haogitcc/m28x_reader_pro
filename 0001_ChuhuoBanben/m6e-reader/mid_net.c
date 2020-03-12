#include "app_sys_setting.h"
#include "dhcpcd.h"
#include "osex_net.h"



static char g_interface[16] = "eth0"; //eth0,rausb0,ppp,eth0.xxxx,rausb0.xxxx



int mid_connect()
{
	int nettype = 0;
	int ret = -1;
  	sysSettingGetInt("nettype", &nettype, 0);
	if(nettype == 0)
		ret = mid_net4_staticip_connect();
	else if(nettype == 1){
		//ret = dhcp_init("M6EReader","1.0.0");
	}else 
		ret = mid_net4_staticip_connect();

	if(ret < 0)
		mid_net4_staticip_connect();

}

int mid_net4_staticip_connect(void)
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

