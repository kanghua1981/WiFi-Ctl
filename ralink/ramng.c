/*
ralink mng
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h> 
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>

#include  <linux/types.h>
#include  <linux/socket.h>
//#include  <linux/if.h>
#include <linux/wireless.h>
#include <netinet/in.h>
//#include <arpa/inet.h>
#include <sys/types.h>
#include "nvram.h"
#include "route.h"
#include "ramng.h"
#include "oid.h"

/*
 * concatenate a string with an integer
 * ex: racat("SSID", 1) will return "SSID1"
 */
char *racat(char *s, int i)
{
	static char str[32];
	snprintf(str, 32, "%s%1d", s, i);
	return str;
}

int _getwanlink(char *link)
{
    int syscmd = "switch reg r 80 | awk -F'=' '{print $3}'";
    FILE *fp = NULL;
    fp = popen(syscmd, "r");
    unsigned int val = 0;
    char result[16] = {0};
	fgets(result, sizeof(result), fp);
	pclose(fp);
    
	val = strtoul(result,NULL,16);
	if (val & 0x20000000)
        sprintf(link,"1");
	else
	    sprintf(link,"0");
    return 0;
}

/*
 * arguments: index - index of the Nth value (starts from 0)
 *            old_values - un-parsed values
 *            new_value - new value to be replaced
 * description: parse values delimited by semicolon,
 *              replace the Nth value with new_value,
 *              and return the result
 * WARNING: return the internal static string -> use it carefully
 */
char *setNthValue(int index, char *old_values, char *new_value)
{
	int i;
	char *p, *q;
	static char ret[2048];
	char buf[8][256];

	memset(ret, 0, 2048);
	for (i = 0; i < 8; i++)
		memset(buf[i], 0, 256);

	//copy original values
	for ( i = 0, p = old_values, q = strchr(p, ';')  ;
	      i < 8 && q != NULL                         ;
	      i++, p = q + 1, q = strchr(p, ';')         )
	{
		strncpy(buf[i], p, q - p);
	}
	strcpy(buf[i], p); //the last one

	//replace buf[index] with new_value
	strncpy(buf[index], new_value, 256);

	//calculate maximum index
	index = (i > index)? i : index;

	//concatenate into a single string delimited by semicolons
	strcat(ret, buf[0]);
	for (i = 1; i <= index; i++) {
		strncat(ret, ";", 2);
		strncat(ret, buf[i], 256);
	}

	return ret;
}


/*
 * substitution of getNthValue which dosen't destroy the original value
 */
int getNthValueSafe(int index, char *value, char delimit, char *result, int len)
{
    int i=0, result_len=0;
    char *begin, *end;

    if(!value || !result || !len)
        return -1;

    begin = value;
    end = strchr(begin, delimit);

    while(i<index && end){
        begin = end+1;
        end = strchr(begin, delimit);
        i++;
    }

    //no delimit
    if(!end){
		if(i == index){
			end = begin + strlen(begin);
			result_len = (len-1) < (end-begin) ? (len-1) : (end-begin);
		}else
			return -1;
	}else
		result_len = (len-1) < (end-begin)? (len-1) : (end-begin);

	memcpy(result, begin, result_len );
	*(result+ result_len ) = '\0';

	return 0;
}


static inline void STFs(int nvram, int index, char *flash_key, char *value)
{
	char *result;
	char *tmp = (char *) nvram_bufget(nvram, flash_key);
	if(!tmp)
		tmp = "";
	result = setNthValue(index, tmp, value);
	nvram_bufset(nvram, flash_key, result);
	
	return ;
}
static inline void LFF(char *value,int nvram,char* flash_key,int index)
{
    char *tmp = (char *) nvram_bufget(nvram, flash_key);
    if (!tmp)
        tmp="";
    getNthValueSafe(index, tmp, ';', value, 32);
    return;
}



int set_wifi_basic(char *channel,char *strenth)
{
    if (!strncmp(channel, "0", 1)) {
		//doSystem("iwpriv ra0 set RadioOn=0");
		//nvram_set(RT2860_NVRAM, "RadioOff", "1");
		//websRedirect(wp, "wireless/basic.asp");
		nvram_bufset(RT2860_NVRAM, "AutoChannelSelect", "1");
	}
	else
	{
	    nvram_bufset(RT2860_NVRAM, "AutoChannelSelect", "0");
	    nvram_bufset(RT2860_NVRAM, "Channel", channel);
	}
	
    nvram_bufset(RT2860_NVRAM, "TxPower", strenth);
    nvram_commit(RT2860_NVRAM);
    return 0;
}

int set_ssid(int index,char *strSsid,char *strSwitch,char *strHide,
                char *strAuthMode,char *strEncryption,char *strPassword)
{
    if (index == 0)
    {
        nvram_bufset(RT2860_NVRAM, "SSID1", strSsid);
        if (strncmp(strSwitch,"ON",2) == 0)
        {
            system("ifconfig ra0 up");
            nvram_set(RT2860_NVRAM, "WiFiOff", "0");
        }
        else
        {
            system("ifconfig ra0 down");
            nvram_set(RT2860_NVRAM, "WiFiOff", "1");
        }
        if (strncmp(strHide,"ON",2)== 0)
        {
            //nvram_set(RT2860_NVRAM, "HideSSID", "1");
            STFs(RT2860_NVRAM,index, "HideSSID", "1");
        }
        else
        {
            //nvram_set(RT2860_NVRAM, "HideSSID", "0");
            STFs(RT2860_NVRAM,index, "HideSSID", "0");
        }
        
        STFs(RT2860_NVRAM,index, "AuthMode", strAuthMode);
        STFs(RT2860_NVRAM,index, "EncrypType", strEncryption);

        nvram_bufset(RT2860_NVRAM, racat("WPAPSK", 1), strPassword);  
        printf("%s\n",strAuthMode);
        printf("%s\n",strEncryption);
    }
    else
    {
        nvram_bufset(RT2860_NVRAM, "SSID2", strSsid);
        if (strncmp(strSwitch,"ON",2) == 0)
        {
            system("ifconfig ra1 up");
            nvram_set(RT2860_NVRAM, "WiFiOff", "0");
        }
        else
        {
            system("ifconfig ra1s down");
            nvram_set(RT2860_NVRAM, "WiFiOff", "1");
        }
        if (strncmp(strHide,"ON",2)== 0)
        {
            //nvram_set(RT2860_NVRAM, "HideSSID", "1");
            STFs(RT2860_NVRAM,index, "HideSSID", "1");
        }
        else
        {
            //nvram_set(RT2860_NVRAM, "HideSSID", "0");
            STFs(RT2860_NVRAM,index, "HideSSID", "0");
        }
        printf("%s\n",strAuthMode);
        printf("%s\n",strEncryption);
        STFs(RT2860_NVRAM,index, "AuthMode", strAuthMode);
        STFs(RT2860_NVRAM,index, "EncrypType", strEncryption);

        nvram_bufset(RT2860_NVRAM, racat("WPAPSK", 2), strPassword);  
    }
    nvram_commit(RT2860_NVRAM);
    return 0;
}


int set_admin(char *admuser,char *admpass)
{
    nvram_bufset(RT2860_NVRAM, "Login", admuser);
	nvram_bufset(RT2860_NVRAM, "Password", admpass);
	nvram_commit(RT2860_NVRAM);
	return 0;
}

int set_default(void)
{
    system("ralink_init clear 2860");
    //system("reboot");
    return 0;
}


int set_dhcpmode()
{
    nvram_bufset(RT2860_NVRAM, "wanConnectionMode", "DHCP");
    nvram_commit(RT2860_NVRAM);
    return 0;
}

int set_staticmode(char *strIP,char *strNetmask,char*strGateway,char *strDNS1,char *strDNS2)
{
    nvram_bufset(RT2860_NVRAM, "wanConnectionMode", "STATIC");
    nvram_bufset(RT2860_NVRAM, "wan_ipaddr", strIP);
    nvram_bufset(RT2860_NVRAM, "wan_netmask", strNetmask);
    nvram_bufset(RT2860_NVRAM, "wan_gateway", strGateway);
    nvram_bufset(RT2860_NVRAM, "wan_primary_dns", strDNS1);
    nvram_bufset(RT2860_NVRAM, "wan_secondary_dns", strDNS2);
    nvram_commit(RT2860_NVRAM);
    return 0;
}

int set_pppoe(char* strUser,char *strPwd)
{
    nvram_bufset(RT2860_NVRAM, "wan_pppoe_user", strUser);
	nvram_bufset(RT2860_NVRAM, "wan_pppoe_pass", strPwd);
	nvram_bufset(RT2860_NVRAM, "wanConnectionMode", "PPPOE");
	nvram_commit(RT2860_NVRAM);
}

int get_wifi_basic(char *Channel,char *Strenth)
{
    char *strChannel,*strStrenth;
    char *value = nvram_bufget(RT2860_NVRAM, "AutoChannelSelect");
    if (0 == strncmp(value, "1", 2))
		strChannel = "0";
    else
	    strChannel = nvram_bufget(RT2860_NVRAM, "Channel");

	strStrenth = nvram_bufget(RT2860_NVRAM, "TxPower");

	strcpy(Channel,strChannel);
	strcpy(Strenth,strStrenth);
	return 0;
}

int get_admin(char *user,char *pwd)
{
    char *admuser,*admpass;
    admuser = nvram_bufget(RT2860_NVRAM, "Login");
	admpass = nvram_bufget(RT2860_NVRAM, "Password");;

    strcpy(user,admuser);
    strcpy(pwd,admpass);
}

int getDhcpCliList(struct DhcpInfo *dhcpinfo,int *num)
{
	FILE *fp;
	int i = 0;
	struct dhcpOfferedAddr {
		unsigned char hostname[16];
		unsigned char mac[16];
		unsigned long ip;
		unsigned long expires;
	} lease;
	struct in_addr addr;
	unsigned long expires;
	unsigned d, h, m;
    printf("enter %s\n",__func__);
	system("killall -q -USR1 udhcpd");

	fp = fopen("/var/udhcpd.leases", "r");
	if (NULL == fp)
		return -1;

	while (fread(&lease, 1, sizeof(lease), fp) == sizeof(lease)) {

        //printf("lease.mac=%x\n",lease.mac);
		sprintf(dhcpinfo[i].hostname, "%s", lease.hostname);
		sprintf(dhcpinfo[i].mac, "%02X:%02X:%02X:%02X:%02X:%02X",
		        lease.mac[0],lease.mac[1],lease.mac[2],lease.mac[3],
		        lease.mac[4],lease.mac[5]);
		addr.s_addr = lease.ip;
		expires = ntohl(lease.expires);
		sprintf(dhcpinfo[i].ip, "%s", inet_ntoa(addr));
		d = expires / (24*60*60); expires %= (24*60*60);
		h = expires / (60*60); expires %= (60*60);
		m = expires / 60; expires %= 60;
		//if (d) websWrite(wp, T("%u days "), d);
		sprintf(dhcpinfo[i].expires, "%02u:%02u:%02u", h, m, (unsigned)expires);
		i++;
	}
	fclose(fp);
	*num = i;
	return 0;
}



/*
 * arguments: ifname  - interface name
 *            if_addr - a 16-byte buffer to store ip address
 * description: fetch ip address, netmask associated to given interface name
 */
int getIfIp(char *ifname, char *if_addr)
{
		struct ifreq ifr;
		int skfd = 0;

		if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
				printf("getIfIp: open socket error");
				return -1;
		}

		strncpy(ifr.ifr_name, ifname, IF_NAMESIZE);
		if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0) {
				close(skfd);
				//error(E_L, E_LOG, T("getIfIp: ioctl SIOCGIFADDR error for %s"), ifname);
				return -1;
		}
		strcpy(if_addr, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

		close(skfd);
		return 0;
}


char* getWanIfNamePPP(void)
{
    const char *cm = nvram_bufget(RT2860_NVRAM, "wanConnectionMode");
    //const char *wan=nvram_bufget(RT2860_NVRAM,"wanport");
    if (!strncmp(cm, "PPPOE", 6) || !strncmp(cm, "L2TP", 5) || !strncmp(cm, "PPTP", 5) 
#ifdef CONFIG_USER_3G
		|| !strncmp(cm, "3G", 3)
#endif
	){
        return "ppp0";
	}

    return "eth2.2";
    //return wan;
}



int getWlanStaInfo(char *ifname,STA_MAC_TABLE *statable)
{
    int i, s;
	struct iwreq iwr;
	RT_802_11_MAC_TABLE table = {0};
	s = socket(AF_INET, SOCK_DGRAM, 0);
	strncpy(iwr.ifr_name, ifname, IFNAMSIZ);
	iwr.u.data.pointer = (caddr_t)&table;

	if (s < 0) {
	    printf("create socket failed\n");
		return -1;
	}
	
	if (ioctl(s, RTPRIV_IOCTL_GET_MAC_TABLE, &iwr) < 0) {
	    printf(" socket ioctl failed\n");
		close(s);
		return -1;
	}
    statable->Num = table.Num;
	for (i = 0; i < table.Num; i++) {
	    /*
		websWrite(wp, T("<tr><td>%02X:%02X:%02X:%02X:%02X:%02X</td>"),
				table.Entry[i].Addr[0], table.Entry[i].Addr[1],
				table.Entry[i].Addr[2], table.Entry[i].Addr[3],
				table.Entry[i].Addr[4], table.Entry[i].Addr[5]);
		websWrite(wp, T("<td>%d</td><td>%d</td><td>%d</td>"),
				table.Entry[i].Aid, table.Entry[i].Psm, table.Entry[i].MimoPs);
		websWrite(wp, T("<td>%d</td><td>%s</td><td>%d</td><td>%d</td></tr>"),
				table.Entry[i].TxRate.field.MCS,
				(table.Entry[i].TxRate.field.BW == 0)? "20M":"40M",
				table.Entry[i].TxRate.field.ShortGI, table.Entry[i].TxRate.field.STBC);
				*/
	    sprintf(statable->entry[i].mac,"%02X:%02X:%02X:%02X:%02X:%02X",table.Entry[i].Addr[0],
	                table.Entry[i].Addr[1],table.Entry[i].Addr[2], table.Entry[i].Addr[3],
				table.Entry[i].Addr[4], table.Entry[i].Addr[5]);
	    sprintf(statable->entry[i].bw,"%s",(table.Entry[i].TxRate.field.BW == 0)? "20M":"40M");
	}


	return 0;

}

int get_wan_mode(char *mode)
{
    char *wanmode;
    wanmode = nvram_bufget(RT2860_NVRAM, "wanConnectionMode");
    strcpy(mode,wanmode);
    return 0;
}

int get_pppoe_info(char *user,char *pwd)
{

    char *strUser,*strPwd;
    strUser = nvram_bufget(RT2860_NVRAM, "wan_pppoe_user");
	strPwd = nvram_bufget(RT2860_NVRAM, "wan_pppoe_pass");
    strcpy(user,strUser);
    strcpy(pwd,strPwd);
    return 0;
}

int get_static_info(char *ip,char *netmask,char *gateway,char *dns1,char *dns2)
{
    char *strIP,*strNetmask,*strGateway,*strDns1,*strDns2;
    strIP = nvram_bufget(RT2860_NVRAM, "wan_ipaddr");
    strNetmask = nvram_bufget(RT2860_NVRAM, "wan_netmask");
    strGateway = nvram_bufget(RT2860_NVRAM, "wan_gateway");
    strDns1 = nvram_bufget(RT2860_NVRAM, "wan_primary_dns");
    strDns2 = nvram_bufget(RT2860_NVRAM, "wan_secondary_dns");

    strcpy(ip,strIP);
    strcpy(netmask,strNetmask);
    strcpy(gateway,strGateway);
    strcpy(dns1,strDns1);
    strcpy(dns2,strDns2);

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
	struct ifreq ifr;
	char *ptr;
	int skfd;

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			printf("getIfMac: open socket error");
			return -1;
	}

	strncpy(ifr.ifr_name, ifname, IF_NAMESIZE);
	if(ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0) {
			close(skfd);
			//error(E_L, E_LOG, T("getIfMac: ioctl SIOCGIFHWADDR error for %s"), ifname);
			return -1;
	}

	ptr = (char *)&ifr.ifr_addr.sa_data;
	sprintf(if_hw, "%02X:%02X:%02X:%02X:%02X:%02X",
					(ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377),
					(ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377));

	close(skfd);
	return 0;
}


/*
 * description: write WAN MAC address accordingly
 */
int getWanMac(char *if_mac)
{
	//char if_mac[18];

	if (-1 == getIfMac("eth2.5", if_mac)) {
		return -1;
	}
	return 0;
}

char  *get_wan_ifname()
{
    return "eth2.5";
}


/*
 * arguments: ifname - interface name
 *            if_net - a 16-byte buffer to store subnet mask
 * description: fetch subnet mask associated to given interface name
 *              0 = bridge, 1 = gateway, 2 = wirelss isp
 */
int getIfNetmask(char *ifname, char *if_net)
{
	struct ifreq ifr;
	int skfd = 0;

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("getIfNetmask:Create socket failed\n");
		return -1;
	}

	strncpy(ifr.ifr_name, ifname, IF_NAMESIZE);
	if (ioctl(skfd, SIOCGIFNETMASK, &ifr) < 0) {
		close(skfd);
		//error(E_L, E_LOG, T("getIfNetmask: ioctl SIOCGIFNETMASK error for %s\n"), ifname);
		return -1;
	}
	strcpy(if_net, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	close(skfd);
	return 0;
}




/*
 * description: write WAN default gateway accordingly
 */
int getWanGateway(char *sgw)
{
	char   buff[256];
	int    nl = 0 ;
	struct in_addr dest;
	struct in_addr gw;
	int    flgs, ref, use, metric;
	unsigned long int d,g,m;
	int    find_default_flag = 0;

	//char sgw[16];

	FILE *fp = fopen("/proc/net/route", "r");

	while (fgets(buff, sizeof(buff), fp) != NULL) {
		if (nl) {
			int ifl = 0;
			while (buff[ifl]!=' ' && buff[ifl]!='\t' && buff[ifl]!='\0')
				ifl++;
			buff[ifl]=0;    /* interface */
			if (sscanf(buff+ifl+1, "%lx%lx%X%d%d%d%lx",
						&d, &g, &flgs, &ref, &use, &metric, &m)!=7) {
				fclose(fp);
				return -1;
			}

			if (flgs&RTF_UP) {
				dest.s_addr = d;
				gw.s_addr   = g;
				strcpy(sgw, (gw.s_addr==0 ? "" : inet_ntoa(gw)));

				if (dest.s_addr == 0) {
					find_default_flag = 1;
					break;
				}
			}
		}
		nl++;
	}
	fclose(fp);

	if (find_default_flag == 1)
		return -1;
	else
		return 0;
}

int get_ssid(int index,char *ssid)
{
    char *strSSID = NULL;
    if (index == 0)
    {
        strSSID = nvram_bufget(RT2860_NVRAM, "SSID1");
    }
    else
    {
        strSSID = nvram_bufget(RT2860_NVRAM, "SSID2");
    }
    
    strcpy(ssid,strSSID);
    return 0;
}

int get_channel(char *channel)
{
    char *strChannel = NULL;
    strChannel = nvram_bufget(RT2860_NVRAM, "Channel");

    strcpy(channel,strChannel);
    return 0;
}


int get_Dns(char *dns1,char *dns2,int *num)
{
	FILE *fp;
	char buf[80] = {0}, ns_str[11], dns[16] = {0};
	int type, idx = 0, req = 0;
    

	fp = fopen("/etc/resolv.conf", "r");
	if (NULL == fp)
		return -1;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (strncmp(buf, "nameserver", 10) != 0)
			continue;
		sscanf(buf, "%s%s", ns_str, dns);
		idx++;
		if (idx == 1)
		    strcpy(dns1,dns);
		else
		    strcpy(dns2,dns);
		
	}
	fclose(fp);
    *num = idx;
	return 0;
}


int get_ssid_authmode(int index,char *auth)
{
    char strauthmode[16] = {0};
    printf("enter %s\n",__func__);
    //LFF(index, RT2860_NVRAM, "HideSSID", 0);

    LFF(strauthmode, RT2860_NVRAM, "AuthMode", index);
    
    strcpy(auth,strauthmode);
    return 0; 
}

int get_ssid_hide(int index,char *hide)
{
    char strhide[3] = {0};
    printf("enter %s\n",__func__);
    LFF(strhide, RT2860_NVRAM, "HideSSID", index);
    if (strcmp(strhide,"0") == 0 )
        strcpy(hide,"OFF");
    else
        strcpy(hide,"ON");
    
    return 0; 
}

int get_ssid_encry(int index,char *encry)
{
    char strEncry[20] = {0};
    printf("enter %s\n",__func__);
    LFF(strEncry,RT2860_NVRAM, "EncrypType", index);
    strcpy(encry,strEncry);
    
    return 0;
}

int get_ssid_password(int index,char *pwd)
{
    char *strPwd;
    printf("enter %s\n",__func__);
    strPwd = nvram_bufget(RT2860_NVRAM,racat("WPAPSK", index+1));
    strcpy(pwd,strPwd);
    return 0;
}

int get_wifi_status(char *status)
{
    char *strSwitch;
    printf("enter %s\n",__func__);
    strSwitch = nvram_bufget(RT2860_NVRAM, "WiFiOff");
    if (strSwitch == NULL)
    {
        strSwitch = "0";
    }
    if (strcmp(strSwitch,"0")== 0)
    {
        strSwitch = "ON";
    }
    else
    {
        strSwitch = "OFF";
    }
    strcpy(status,strSwitch);
    return 0;
}

void restart_network(void)
{
    system("killall goahead");
    system("goahead &");
}

void sysreboot(void)
{
    system("reboot");
}

