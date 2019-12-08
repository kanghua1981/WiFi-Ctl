
/*
test mng
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h> 
#include <sys/types.h>
#include "route.h"

int set_wifi_basic(char *channel,char *strenth)
{
    printf("%s:%s\n",__FILE__,__func__);
    printf("channel:%s\n",channel);
    printf("strenth:%s\n",strenth);
    return 0;
}

int set_ssid(int index,char *strSsid,char *strSwitch,char *strHide,
                char *strAuthMode,char *strEncryption,char *strPassword)
{
    printf("%s:%s\n",__FILE__,__func__);
    printf("index:%d\n",index);
    printf("strSsid:%s\n",strSsid);
    printf("strSwitch:%s\n",strSwitch);
    printf("strHide:%s\n",strHide);
    printf("strAuthMode:%s\n",strAuthMode);
    printf("strEncryption:%s\n",strEncryption);
    printf("strPassword:%s\n",strPassword);
    
    return 0;
}


int set_admin(char *admuser,char *admpass)
{
    printf("%s:%s\n",__FILE__,__func__);
    printf("admuser:%s\n",admuser);
    printf("admpass:%s\n",admpass);
	return 0;
}

int set_default()
{
    printf("%s:%s\n",__FILE__,__func__);
    return 0;
}


int set_dhcpmode()
{
    printf("%s:%s\n",__FILE__,__func__);
    return 0;
}

int set_staticmode(char *strIP,char *strNetmask,char*strGateway,char *strDNS1,char *strDNS2)
{
    printf("%s:%s\n",__FILE__,__func__);
    printf("strIP:%s\n",strIP);
    printf("strNetmask:%s\n",strNetmask);
    printf("strGateway:%s\n",strGateway);
    printf("strDNS1:%s\n",strDNS1);
    printf("strDNS2:%s\n",strDNS2);
    return 0;
}

int set_pppoe(char* strUser,char *strPwd)
{
    printf("%s:%s\n",__FILE__,__func__);
    printf("strUser:%s\n",strUser);
    printf("strPwd:%s\n",strPwd);
}


int get_wifi_basic(char *Channel,char *Strenth)
{
    printf("%s:%s\n",__FILE__,__func__);
	strcpy(Channel,"1");
	strcpy(Strenth,"100");
	return 0;
}

int get_admin(char *user,char *pwd)
{
    printf("%s:%s\n",__FILE__,__func__);
    strcpy(user,"admin");
    strcpy(pwd,"admin");
    return 0;
}



int getDhcpCliList(struct dhcpinfo *dhcpinfo,int *num)
{
    struct dhcpOfferedAddr *pdhcp = dhcpinfo;
	printf("%s:%s\n",__FILE__,__func__);
	*num = 3;

	
    strcpy(dhcpinfo[0].hostname,"111");
    strcpy(dhcpinfo[0].expires,"12:01:01");
    strcpy(dhcpinfo[0].ip,"192.168.1.1");
    strcpy(dhcpinfo[0].mac,"00:01:02:03:04:05");

    strcpy(dhcpinfo[1].hostname,"111");
    strcpy(dhcpinfo[1].expires,"12:01:01");
    strcpy(dhcpinfo[1].ip,"192.168.1.1");
    strcpy(dhcpinfo[1].mac,"00:01:02:03:04:05");

    strcpy(dhcpinfo[2].hostname,"111");
    strcpy(dhcpinfo[2].expires,"12:01:01");
    strcpy(dhcpinfo[2].ip,"192.168.1.1");
    strcpy(dhcpinfo[2].mac,"00:01:02:03:04:05");
	//strcpy();
	return 0;
}



/*
 * arguments: ifname  - interface name
 *            if_addr - a 16-byte buffer to store ip address
 * description: fetch ip address, netmask associated to given interface name
 */
int getIfIp(char *ifname, char *if_addr)
{
	printf("%s:%s\n",__FILE__,__func__);
	printf("ifname=%s",ifname);
	strcpy(if_addr,"192.168.1.1");
	return 0;
}


char* getWanIfNamePPP(void)
{
    printf("%s:%s\n",__FILE__,__func__);
    return "eth2.2";
}



int getWlanStaInfo(char *ifname,STA_MAC_TABLE *table)
{
    strcpy(table->entry[0].mac,"00:02:03:04:05:03");
    strcpy(table->entry[0].bw ,"40M");
    strcpy(table->entry[1].mac,"00:03:03:04:05:03");
    strcpy(table->entry[1].bw ,"20M");
    strcpy(table->entry[2].mac,"00:04:03:04:05:03");
    strcpy(table->entry[2].bw ,"40M");
    table->Num = 3;

	return 0;
}


int get_wan_mode(char *mode)
{
    printf("%s:%s\n",__FILE__,__func__);
    strcpy(mode,"STATIC");
    return 0;
}

int get_pppoe_info(char *user,char *pwd)
{
    printf("%s:%s\n",__FILE__,__func__);
    strcpy(user,"pppoe");
    strcpy(pwd,"pppoe");
    return 0;
}


int get_static_info(char *ip,char *netmask,char *gateway,char *dns1,char *dns2)
{
    printf("%s:%s\n",__FILE__,__func__);
    strcpy(ip,"192.168.168.123");
    strcpy(netmask,"255.255.255.0");
    strcpy(gateway,"192.168.1.1");
    strcpy(dns1,"192.168.168.1");
    strcpy(dns2,"8.8.8.8");
    return 0;
}

/*
 * description: write WAN ip address accordingly
 */
int getWanIp(char *if_addr)
{
	if (-1 == getIfIp(getWanIfNamePPP(), if_addr)) {
		//websError(wp, 500, T("getWanIp: calling getIfIp error\n"));
		return -1;
	}
	return 0;
}

/*
 * arguments: ifname  - interface name
 *            if_addr - a 18-byte buffer to store mac address
 * description: fetch mac address according to given interface name
 */
int getIfMac(char *ifname, char *if_hw)
{
    printf("%s:%s\n",__FILE__,__func__);
	printf("ifname=%s\n",ifname);
    strcpy(if_hw,"00:01:02:03:04:05");
	return 0;
}


/*
 * description: write WAN MAC address accordingly
 */
int getWanMac(char *if_mac)
{
	//char if_mac[18];

	if (-1 == getIfMac("eth2.2", if_mac)) {
		return -1;
	}
	return 0;
}

char  *get_wan_ifname()
{
    return "eth2.2";
}

/*
 * arguments: ifname - interface name
 *            if_net - a 16-byte buffer to store subnet mask
 * description: fetch subnet mask associated to given interface name
 *              0 = bridge, 1 = gateway, 2 = wirelss isp
 */
int getIfNetmask(char *ifname, char *if_net)
{
	printf("%s:%s\n",__FILE__,__func__);
	printf("ifname=%s\n",ifname);
    strcpy(if_net,"255.255.255.0");
	return 0;
}


/*
 * description: write WAN default gateway accordingly
 */
int getWanGateway(char *sgw)
{
	printf("%s:%s\n",__FILE__,__func__);
	strcpy(sgw,"192.168.168.1");
	return 0;
}

int get_ssid(int index,char *ssid)
{
    printf("%s:%s\n",__FILE__,__func__);
    if (index == 0)
        strcpy(ssid,"ssid1");
    else
        strcpy(ssid,"ssid2");
    return 0;
}

int get_channel(char *channel)
{
    printf("%s:%s\n",__FILE__,__func__);
    strcpy(channel,"9");
    return 0;
}


int get_Dns(char *dns1,char *dns2)
{
    printf("%s:%s\n",__FILE__,__func__);
    strcpy(dns1,"192.168.1.1");
    strcpy(dns1,"8.8.8.8");
	return 0;
}



int get_ssid_authmode(int index,char *auth)
{
    printf("%s:%s\n",__FILE__,__func__);
    
    strcpy(auth,"WPA2PSK");
    return 0; 
}

int get_ssid_hide(int index,char *hide)
{
    printf("%s:%s\n",__FILE__,__func__);
    strcpy(hide,"ON");
    return 0; 
}

int get_ssid_encry(int index,char *encry)
{
    printf("%s:%s\n",__FILE__,__func__);
    strcpy(encry,"TKIP");
    return 0;
}

int get_ssid_password(int index,char *pwd)
{
    printf("%s:%s\n",__FILE__,__func__);
    strcpy(pwd,"12345678");
    return 0;
}

int get_wifi_status(char *status)
{
    printf("%s:%s\n",__FILE__,__func__);
    strcpy(status,"1");
    return 0;
}



