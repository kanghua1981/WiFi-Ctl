

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/route.h>
#include "route.h"


typedef struct tagPowerSaveMode
{
    char sleeptime[32];
    char wakeuptime[32];
    int  repeatMode;   /*mask 方式来定义星期*/
}PowerSaveMode;

typedef struct tagPowerSaveList
{
    int enable;
    PowerSaveMode sleepinfo[32];
    int num;
}PowerSaveList;

typedef struct tagSSIDInfo
{
    char ssidname[32];
    char ssidpwd[64];
    int  channel;
    char encryption[4];
    int  power;
    int  onoff;
    int  maxsta;
    int hide;
    PowerSaveList sleeplist;
}SSIDInfo;


extern FILE *logfile;
#define log(format,str...)   \
{\
    fprintf(logfile,"{%s:%d} "format,__FILE__,__LINE__,##str); \
    fflush(logfile); \
}

#define AP_MSG_OK                   0x0
#define AP_MSG_NULL_POINT           0xffff0001
#define AP_MSG_INVILID_ENCRYPTION   0xffff0002
#define AP_MSG_INVILID_POWER        0xffff0003
#define AP_MSG_INVILID_ONOFF        0xffff0003

#define AP_MSG_INVILID_PARAM        0xffff0029
#define AP_MSG_SYSTEM_ERR           0xffff0030


#define ENCRYPT_TYPE_NONE       0
#define ENCRYPT_TYPE_WPA        1
#define ENCRYPT_TYPE_WEP        2


#define AP_SSID_1                   0
#define STA_SSID                    4

int get_wan_name(char *ifname)
{
    FILE *pp;
    char tmpbuffer[32] = {0};
    
    pp = popen("/sbin/getwandevice.sh","r");
    if (pp == NULL)
    {
        return -1;
    }
    fgets(tmpbuffer,32,pp);
    sscanf(tmpbuffer,"%[^\n]",ifname);
    pclose(pp);
    
    return 0;
}

char *get_wan_ifname()
{
    static char wan_name[32] = {0};
    get_wan_name(wan_name);
    return wan_name;
}


int setWiFiinfo(SSIDInfo *ssidinfo)
{
    char cmd[64] = {0};
    char tmpbuffer[16] = {0};
    int power;
    int i = 0;
    char encryption[16] = {0};
    int encrypt_type =ENCRYPT_TYPE_NONE;
    int mask;
    if (NULL == ssidinfo)
        return AP_MSG_NULL_POINT;

    //printf("ssidinfo->encryption=%s\n",ssidinfo->encryption);
    if (0 == strncmp(ssidinfo->encryption,"0",1))
    {
        strcpy(encryption,"none");
    }
    else if (0 == strncmp(ssidinfo->encryption,"1",1))
    {
        encrypt_type=ENCRYPT_TYPE_WEP;
        strcpy(encryption,"wep");
    }
    else if (0 == strncmp(ssidinfo->encryption,"2",1))
    {
        encrypt_type=ENCRYPT_TYPE_WPA;
        strcpy(encryption,"wpa-psk");

    }
    else if (0 == strncmp(ssidinfo->encryption,"3",1))
    {
        encrypt_type=ENCRYPT_TYPE_WPA;
        strcpy(encryption,"wpa-psk2");
    }
    else if (0 == strncmp(ssidinfo->encryption,"4",1))
    {
        encrypt_type=ENCRYPT_TYPE_WPA;
        strcpy(encryption,"mixed-psk");
    }
    else
    {
        log("invalid encryption\n");
        return AP_MSG_INVILID_ENCRYPTION;
    }

    
    power = ssidinfo->power;
    if (power < 0 || power > 100)
    {
        log("invalid power\n");
        return AP_MSG_INVILID_POWER;
    }
    if (encrypt_type == ENCRYPT_TYPE_WPA)
    {
        if (strlen(ssidinfo->ssidpwd) < 8)
        {
            log("ssid password is too short\n");
            return -1;
        }
        sprintf(cmd,"uci set wireless.SSID1.key=%s",ssidinfo->ssidpwd);
        system(cmd);
        sprintf(cmd,"uci set wireless.SSID1.encryption=%s",encryption);
        system(cmd);
    }
    else if (encrypt_type == ENCRYPT_TYPE_WEP)
    {
        sprintf(cmd,"uci set wireless.SSID1.key1=%s",ssidinfo->ssidpwd);
        system(cmd);
        sprintf(cmd,"uci set wireless.SSID1.encryption=%s",encryption);
        system(cmd);
    }
    else
    {
        sprintf(cmd,"uci set wireless.SSID1.encryption=%s",encryption);
        system(cmd);
    }

    if (ssidinfo->channel == -1)
    {
        sprintf(cmd,"uci set wireless.radio0.channel=auto");
        system(cmd);
    }
    else if (ssidinfo->channel > 0 && ssidinfo->channel < 14 )
    {
        sprintf(cmd,"uci set wireless.radio0.channel=%d",ssidinfo->channel);
        system(cmd);
    }
    else
    {
        log("ssid channel error\n");
        return -1;
    }
    
    sprintf(cmd,"uci set wireless.SSID1.ssid=%s",ssidinfo->ssidname);
    system(cmd);
    if (ssidinfo->maxsta != -1)
    {
        sprintf(cmd,"uci set wireless.SSID1.maxassoc=%d",ssidinfo->maxsta);
        system(cmd);
    }

    sprintf(cmd,"uci set wireless.SSID1.hide=%d",ssidinfo->hide);
    system(cmd);
    

    
    if(ssidinfo->onoff == 1)
    {
        //sprintf("uci set wireless.SSID1.disabled=0"
        system("uci set wireless.SSID1.disabled=0");
    }
    else if (ssidinfo->onoff == 0)
    {
        //"uci set wireless.SSID1.disabled=1"
        system("uci set wireless.SSID1.disabled=1");
    }
    else
    {
        log("invalid onoff\n");
        return AP_MSG_INVILID_ONOFF;
    }
    
    
#if 0
    if ( ssidinfo->sleeplist.enable == 0)
    {
         sprintf(cmd,"/sbin/wifischd.sh deleteall");
         system(cmd);
    }
    else
    {
       sprintf(cmd,"/sbin/wifischd.sh deleteall");
       system(cmd);
       i=0;
       while (i < ssidinfo->sleeplist.num)
       {
            int tmp;
           char repeat[32] = {0};
           mask = ssidinfo->sleeplist.sleepinfo[i].repeatMode;
           for (tmp = 0;tmp < 7;tmp++)
           {
                if ((mask >> tmp) & 0x1)
                {
                    if (tmp == 0)
                        sprintf(repeat,"%d",tmp+1);
                    else
                    {
                        sprintf(tmpbuffer,",%d",tmp+1);
                        strcat(repeat,tmpbuffer);
                        memset(tmpbuffer,0,16);
                    }
                }
           }
           sprintf(cmd,"/sbin/wifischd.sh add %s %s %s %s",repeat,
                    ssidinfo->sleeplist.sleepinfo[i].sleeptime,
                    ssidinfo->sleeplist.sleepinfo[i].wakeuptime,
                    "SSID1");
           system(cmd);
           i++;
       }
    }
    system("/sbin/wifischd.sh update");
#endif
    system("uci commit wireless");
    //system("wifi");
    return AP_MSG_OK;
}



int getWiFiChannel(char *ifname)
{
    FILE *pp = NULL;
    char cmd[64] = {0};
    char buffer[8] = {0};
    int channel = 1;
    sprintf(cmd,"getssidchannel.sh %s",ifname);
    pp = popen(cmd,"r");
    if (pp == NULL)
    {
        
        return -1;
    }

    fgets(buffer,8,pp);
    pclose(pp);
    
    if (strlen(buffer) == 0)
        return -1;

    sscanf(buffer,"%d",&channel);
    
    return channel;

}


int getConnectMode()
{
    FILE *fp = NULL;
    char buffer[32] = {0};
    fp = fopen("/etc/config/mode","r");
    if (fp ==  NULL)
    {
        return -1;
    }
    fgets(buffer,32,fp);
    if (strlen(buffer) == 0)
    {
        return -1;
    }
    fclose(fp);
    if (strncmp(buffer,"wired",5) == 0)
    {
        return CONNECT_WIRED_MODE;
    }
    else if (strncmp(buffer,"wireless",8) == 0)
    {
        return CONNECT_WIRELESS_MODE;
    }
    return -1;
}


int setConnectMode(int mode)
{
    FILE *fp = NULL;
    char buffer[32] = {0};
    char cmd[64] = {0};
    //printf("setConnectMode\n");
    fp = fopen("/etc/config/mode","w+");
    if (fp ==  NULL)
    {
        return -1;
    }
    
    if (mode == CONNECT_WIRED_MODE)
    {
        sprintf(cmd,"/sbin/uci delete wireless.SSID4");
        system(cmd);
        sprintf(cmd,"/sbin/uci commit wireless");
        system(cmd);
        sprintf(buffer,"wired");
    }
    else if (mode == CONNECT_WIRELESS_MODE)
    {
        sprintf(cmd,"/sbin/uci delete network.wan");;
        system(cmd);
        
        sprintf(cmd,"/sbin/uci set network.wan=interface");;
        system(cmd);
        
        sprintf(cmd,"/sbin/uci set network.wan.proto=%s","dhcp");
        system(cmd);
        
        sprintf(cmd,"/sbin/uci commit network");
        system(cmd);
        sprintf(buffer,"wireless");
    }
    
    fputs(buffer,fp);
    fflush(fp);
    fclose(fp);
    return 0;

}

void sysreboot()
{
    system("reboot");
}

void restart_network()
{
    system("/sbin/reloadnetwork.sh restart");
}

//set_staticmode(strIP,strNetmask,strGateway,strDNS1,strDNS2);
int set_staticmode(char *strIp,char *strNetmask,char *strGateway,
                        char *strDNS1,char *strDNS2)
{
    char cmd[64] = {0};
    sprintf(cmd,"uci delete network.wan");;
    system(cmd);
    sprintf(cmd,"uci set network.wan=interface");;
    system(cmd);
    sprintf(cmd,"uci set network.wan.ifname=eth1");;
    system(cmd);
    sprintf(cmd,"uci set network.wan.proto=%s","static");
    system(cmd);
    sprintf(cmd,"uci set network.wan.ipaddr=%s",strIp);
    system(cmd);
    sprintf(cmd,"uci set network.wan.netmask=%s",strNetmask);
    system(cmd);
    sprintf(cmd,"uci set network.wan.gateway=%s",strGateway);
    system(cmd);
    sprintf(cmd,"uci set network.wan.dns='%s %s'",strDNS1,strDNS2);
    system(cmd);

    system("uci commit network");
    //system("/sbin/reloadnetwork.sh network");
    system("rm -rf /var/resolv.conf.auto");
    setConnectMode(CONNECT_WIRED_MODE);
    return 0;
}


int set_pppoe(char *usr,char *pwd)
{
    char cmd[64] = {0};
    sprintf(cmd,"uci delete network.wan");;
    system(cmd);
    sprintf(cmd,"uci set network.wan=interface");;
    system(cmd);
    sprintf(cmd,"uci set network.wan.ifname=eth1");;
    system(cmd);
    sprintf(cmd,"uci set network.wan.proto=%s","pppoe");
    system(cmd);
    sprintf(cmd,"uci set network.wan.username=%s",usr);
    system(cmd);
    sprintf(cmd,"uci set network.wan.password=%s",pwd);
    system(cmd);

    system("uci commit network");
    //system("/sbin/reloadnetwork.sh network");
    setConnectMode(CONNECT_WIRED_MODE);
    return 0;
}


int set_dhcpmode()
{
    char cmd[64] = {0};
    sprintf(cmd,"uci delete network.wan");;
    system(cmd);
    sprintf(cmd,"uci set network.wan=interface");;
    system(cmd);
    sprintf(cmd,"uci set network.wan.ifname=eth1");;
    system(cmd);
    sprintf(cmd,"uci set network.wan.proto=%s","dhcp");
    system(cmd);

    system("uci commit network");
    //system("/sbin/reloadnetwork.sh network");
    setConnectMode(CONNECT_WIRED_MODE);
    return 0;
}

int set_default()
{
    system("firstboot");
    return 0;
}

/*set_ssid(index,strSsid,strSwitch,strHide,strAuthMode,strEncryption,strPassword);*/
int set_ssid(int index,char *strSsid,char *strSwitch,char *strHide,
                char *strAuthMode,char *strEncryption,char *strPassword,char *strClientNum)

{
    SSIDInfo ssidinfo;
    
    if (index == AP_SSID_1)
    {
        memset(&ssidinfo,0,sizeof(ssidinfo));
        strcpy(ssidinfo.ssidname,strSsid);
        if (strcmp(strSwitch,"ON") == 0)
        {
            ssidinfo.onoff = 1;
        }
        else
        {
            ssidinfo.onoff = 0;
        }
        //WPAPSKWPA2PSK/ WPA-PSK/WPA2-PSK/OPENWEP/Disable
        if (strcmp(strAuthMode,"WPAPSKWPA2PSK") == 0)
        {
            strcpy(ssidinfo.encryption,"4");
        }
        else if (strcmp(strAuthMode,"WPA-PSK") == 0)
        {
            strcpy(ssidinfo.encryption,"3");
        }
        else if (strcmp(strAuthMode,"WPA2-PSK") == 0)
        {
            strcpy(ssidinfo.encryption,"2");
        }
        else if (strcmp(strAuthMode,"OPENWEP") == 0)
        {
            strcpy(ssidinfo.encryption,"1");
        }
        else if (strcmp(strAuthMode,"OPEN") == 0)
        {
            strcpy(ssidinfo.encryption,"0");
        }
        //get_channel(char * channel)
        ssidinfo.channel = -1;
        strcpy(ssidinfo.ssidpwd,strPassword);
        ssidinfo.sleeplist.enable = 0;
        if (strlen(strClientNum) > 0)
        {
            ssidinfo.maxsta = atoi(strClientNum);
        }
        else 
        {
            ssidinfo.maxsta = -1;
        }
        
        if (strstr(strHide,"OFF") > 0)
        {
            ssidinfo.hide = 1;
        }
        else if (strstr(strHide,"ON") > 0)
        {
            ssidinfo.hide = 0;
        }
        
        setWiFiinfo(&ssidinfo);
    }
    //system("/sbin/reloadnetwork.sh wifi");
    return 0;
}


/*set_wifi_basic(channel,strenth);*/
int set_wifi_basic(char *channel,char *strenth)
{
    char cmd[64] = {0};
    if (strcmp(channel,"0") == 0)
        sprintf(cmd,"uci set wireless.radio0.channel=auto");
    else
        sprintf(cmd,"uci set wireless.radio0.channel=%s",channel);
    
    system(cmd);
    system("uci commit wireless");
    //system("/sbin/reloadnetwork.sh wifi");
    return 0;
}

int set_admin(char *usr,char *pwd)
{
    
    return 0;
}

int get_wifi_basic(char *channel,char *strStrenth)
{
    int ichn;
    ichn = getWiFiChannel("SSID1");
    sprintf(channel,"%d",ichn);
    sprintf(strStrenth,"100",strStrenth);
    return;
}


int getWiFiEncryption(char *ifname,char *encry)
{
    FILE *pp = NULL;
    char cmd[64] = {0};
    char buffer[32] = {0};
    sprintf(cmd,"getssidencry.sh %s",ifname);
    pp = popen(cmd,"r");
    if (pp == NULL)
    {
        return -1;
    }

    fgets(buffer,32,pp);
    pclose(pp);
    
    if (strlen(buffer) == 0)
        return -1;

    sscanf(buffer,"%[^\n]",encry);
    
    return 0;
}

int getSSIDName(char *ssid,char *name)
{
    FILE *pp = NULL;
    char cmd[64] = {0};
    char buffer[64] = {0};
    int channel = 1;
    sprintf(cmd,"getssidname.sh %s",ssid);
    pp = popen(cmd,"r");
    if (pp == NULL)
    {
        return -1;
    }

    fgets(buffer,64,pp);
    pclose(pp);
    
    if (strlen(buffer) == 0)
        return -1;

    sscanf(buffer,"%[^\n]",name);
    
    return 0;

}



static int GetIpAddr(const char *relname,char *ipaddr)
{
    struct ifreq ifr;
	int sd;
	struct sockaddr_in *addr;
	
	strncpy(ifr.ifr_name, relname, IFNAMSIZ);
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
        return -1;

	if(ioctl(sd,SIOCGIFADDR, &ifr))
	{
	    //log("get addr %s error",relname);
	    //perror("");
	    sprintf(ipaddr,"0.0.0.0");
		return -1;
	}

	addr = ((struct sockaddr_in *) &(ifr.ifr_addr));
	inet_ntop(AF_INET, &(addr->sin_addr), ipaddr, 16);
	return 0;
}


int get_pppoe_info(char *user,char *pwd)
{
    FILE *fp;
    char tmpbuffer[32];
    fp=popen("uci get network.wan.username","r");
    if (fp == NULL)
    {
        log("open pipe failed\n");
        return -1;
    }
    fgets(tmpbuffer,32,fp);
    sscanf(tmpbuffer,"%[^\n]",user);
    pclose(fp);

    fp=popen("uci get network.wan.password","r");
    if (fp == NULL)
    {
        log("open pipe failed\n");
        return AP_MSG_SYSTEM_ERR;
    }
    fgets(tmpbuffer,32,fp);
    sscanf(tmpbuffer,"%[^\n]",pwd);
    pclose(fp);
    
    return 0;
}

int get_static_info(char *ip,char *netmask,char *gateway,char *dns1,char *dns2)
{
    FILE *fp;
    char tmpbuffer[64];
    char multidns[40];
    fp=popen("uci get network.wan.ipaddr","r");
    if (fp == NULL)
    {
        log("open pipe failed\n");
        return -1;
    }
    fgets(tmpbuffer,64,fp);
    sscanf(tmpbuffer,"%[^\n]",ip);
    pclose(fp);

    fp=popen("uci get network.wan.netmask","r");
    if (fp == NULL)
    {
        log("open pipe failed\n");
        return -1;
    }
    fgets(tmpbuffer,64,fp);
    sscanf(tmpbuffer,"%[^\n]",netmask);
    pclose(fp);

    fp=popen("uci get network.wan.gateway","r");
    if (fp == NULL)
    {
        log("open pipe failed\n");
        return -1;
    }
    fgets(tmpbuffer,64,fp);
    sscanf(tmpbuffer,"%[^\n]",gateway);
    pclose(fp);

    fp=popen("uci get network.wan.dns","r");
    if (fp == NULL)
    {
        log("open pipe failed\n");
        return -1;
    }
    fgets(tmpbuffer,64,fp);
    sscanf(tmpbuffer,"%[^\n]",multidns);
    sscanf(multidns,"%s %s",dns1,dns2);
    pclose(fp);
    
    return 0;
}

int get_admin(char *user,char *pwd)
{
    strcpy(user,"root");
    return 0;

}

int get_ssid_encry(int index,char *encry)
{
    sprintf(encry,"TKIP");
    return 0;
}

int get_ssid_password(int index,char *pwd)
{
    char cmd[32] = {0};
    char buffer[32] = {0};
    FILE *pp;
    if (index == AP_SSID_1)
    {
        sprintf(cmd,"uci -q get wireless.SSID1.key");
        pp = popen(cmd,"r");
        if (pp == NULL)
        {
            return -1;
        }
        fgets(buffer,32,pp);
        pclose(pp);
        if (strlen(buffer) == 0)
        {
            return -1;
        }
        sscanf(buffer,"%[^\n]",pwd);
    }
    else if (index == STA_SSID)
    {
        sprintf(cmd,"uci -q get wireless.SSID4.key");
        pp = popen(cmd,"r");
        if (pp == NULL)
        {
            return -1;
        }
        fgets(buffer,32,pp);
        pclose(pp);
        if (strlen(buffer) == 0)
        {
            return -1;
        }
        sscanf(buffer,"%[^\n]",pwd);
    }
    
    return 0;
}


int _getwanlink(char *link)

{
    FILE *pp;
    char cmd[32] = {0};
    char buffer[32] = {0};
    sprintf(cmd,"/sbin/getethlink.sh %s",get_wan_ifname());
    pp = popen(cmd,"r");
    if (pp == NULL)
        return 0;
    fgets(buffer,8,pp);
   // printf("%s:%s\n",ifname,buffer);
    pclose(pp);
    if (strncmp(buffer,"up",2)==0)
    {
        //printf("getphylink:up\n");
        sprintf(link,"1");
    }
    else
    {
        //printf("getphylink:down\n");
        sprintf(link,"0");
        //return 0;
    }
    
    return 0;
}




int get_wan_mode(char *mode)
{
    char cmd[32] = {0};
    char buffer[32] = {0};
    FILE *pp = NULL;
    
    /*如果是无线模式，只支持DHCP方式*/
    if (getConnectMode() == CONNECT_WIRELESS_MODE)
    {
        strcpy(mode,"DHCP");
        return 0;
    }
    sprintf(cmd,"uci -q get network.wan.proto");
    pp = popen(cmd,"r");
    if (pp == NULL)
    {
        return -1;
    }
    fgets(buffer,32,pp);
    pclose(pp);
    if (strlen(buffer) == 0)
    {
        return -1;
    }
    if (strstr(buffer,"dhcp") > 0)
    {
        strcpy(mode,"DHCP");
    }
    else if (strstr(buffer,"pppoe") > 0)
    {
        strcpy(mode,"PPPOE");
    }
    else if (strstr(buffer,"static") > 0)
    {
        strcpy(mode,"STATIC");
    }
    return 0;
}

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
get_ssid(index,strssid);
get_ssid_authmode(index,strAuth);
get_ssid_hide(index,strHide);
*/

int get_ssid(int index,char *ssid)
{
    int ret = 0;
    if (index == AP_SSID_1)
    {
        ret = getSSIDName("SSID1",ssid);
    }
    else if (index == STA_SSID)
    {
        ret = getSSIDName("SSID4",ssid);
    }
    return ret;
}

int get_ssid_authmode(int index,char *strAuth)
{
    char openwrtstrAuth[32] = {0};
    if (index == AP_SSID_1)
    {
        getWiFiEncryption("SSID1",openwrtstrAuth);
        if (strstr(openwrtstrAuth,"none") > 0)
        {
            sprintf(strAuth,"%s","Disable");
        }
        else if (strstr(openwrtstrAuth,"WPA PSK") > 0)
        {
            sprintf(strAuth,"%s","WPA-PSK");
        }
        else if (strstr(openwrtstrAuth,"WPA2 PSK") > 0)
        {
            sprintf(strAuth,"%s","WPA2-PSK");
        }
        else if (strstr(openwrtstrAuth,"WPA2 PSK") > 0)
        {
            sprintf(strAuth,"%s","WPA2-PSK");
        }
        else if (strstr(openwrtstrAuth,"mixed WPA/WPA2 PSK") > 0)
        {
            sprintf(strAuth,"%s","WPAPSKWPA2PSK");
        }
        else if (strstr(openwrtstrAuth,"WEP Open/Shared") > 0)
        {
            sprintf(strAuth,"%s","OPENWEP");
        }
    }
    else if (index == STA_SSID)
    {
        getWiFiEncryption("SSID4",openwrtstrAuth);
        if (strstr(openwrtstrAuth,"none") > 0)
        {
            sprintf(strAuth,"%s","Disable");
        }
        else if (strstr(openwrtstrAuth,"WPA PSK") > 0)
        {
            sprintf(strAuth,"%s","WPA-PSK");
        }
        else if (strstr(openwrtstrAuth,"WPA2 PSK") > 0)
        {
            sprintf(strAuth,"%s","WPA2-PSK");
        }
        else if (strstr(openwrtstrAuth,"WPA2 PSK") > 0)
        {
            sprintf(strAuth,"%s","WPA2-PSK");
        }
        else if (strstr(openwrtstrAuth,"mixed WPA/WPA2 PSK") > 0)
        {
            sprintf(strAuth,"%s","WPAPSKWPA2PSK");
        }
        else if (strstr(openwrtstrAuth,"WEP Open/Shared") > 0)
        {
            sprintf(strAuth,"%s","OPENWEP");
        }
    }
    return 0;
}


int get_ssid_hide(int index,char *strHide)
{
    strcpy(strHide,"none");
    return 0;
}

int getWanIp(char *if_addr)
{
    char ifname[32];
    get_wan_name(ifname);
    return GetIpAddr(ifname,if_addr);
}

int getWlanStaInfo(char *ifname,STA_MAC_TABLE *mactable)
{
    FILE *pp = NULL;
    char buffer[256] = {0};
    char cmd[32] = {0};
    int i = 0;
    sprintf(cmd,"/sbin/getclient.sh %s",ifname);
    pp = fopen(cmd,"r");
    if (pp == NULL)
        return -1;
    while (feof(pp) == 0)
    {
        fgets(buffer,256,pp);
        if (strlen(buffer) > 0)
        {
            scanf(buffer,"%*s %s %*[^\n]",mactable->entry[i].mac);
            sprintf(mactable->entry[i].bw,"20M");
            i++;
        }
    }
    mactable->Num = i;
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


int getWanMac(char *if_mac)
{
    char ifname[32];
    get_wan_name(ifname);
	if (-1 == getIfMac(ifname, if_mac)) {
		return -1;
	}
	return 0;
}


int get_wifi_status(char *status)
{
    char *strSwitch;
    strcpy(status,"ON");
    return 0;
}

int get_channel(char *channel)
{
    int ichn;
    
    ichn = getWiFiChannel("SSID1");
    sprintf(channel,"%d",ichn);
    return 0;
}


int get_Dns(char *dns1,char *dns2,int *num)
{
	FILE *fp;
	char buf[80] = {0}, ns_str[11], dns[16] = {0};
	int type, idx = 0, req = 0;
    

	fp = fopen("/var/resolv.conf.auto", "r");
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


/*
 * description: write WAN default gateway accordingly
 */
int getWanGateway(char *sgw)
{
#if 0
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
#else
    FILE *pp = NULL;
    char buffer[32] = {0};
    pp = popen("route | grep default | awk '{print $2}'","r");
    fgets(buffer,32,pp);
    if (strlen(buffer) == 0)
        return -1;
    sscanf(buffer,"%[^\n]",sgw);
    return 0;

#endif
}

void deletequot(char *str)
{
    char newstr[128] = {0};
    char *kkk = newstr;
    char *tmp = str;
    while (*tmp != '\0')
    {
        if (*tmp == '"')
        {
            tmp++;
        }
        *kkk++ = *tmp++;
        //tmp++;
    }
    strcpy(str,newstr);
    return ;
}


int getScanApInfo(SCAN_AP_TABLE *aptable)
{

    FILE *pp = NULL;
    int i = 0;
    char buffer[256] = {0};
    char auth[32] = {0};
    pp = popen("/sbin/getscaninfo.sh","r");
    if (pp == NULL)
    {
        printf("popen failed\n");
        return -1;
    }
    while (feof(pp) == 0)
    {
        memset(buffer,0,256);
        fgets(buffer,256,pp);
        printf(buffer);
        if (strlen(buffer) > 0)
        {
            sscanf(buffer,"%[^|]|%[^|]|%[^|]|%[^|]|%s",aptable->entry[i].bssid,
                aptable->entry[i].ssid,aptable->entry[i].channel,auth,
                aptable->entry[i].signal);
            deletequot(aptable->entry[i].ssid);

            if (strstr(auth,"CCMP") > 0)
            {
                sprintf(aptable->entry[i].encryption,"%s","AES");
            }
            else
            {
                sprintf(aptable->entry[i].encryption,"%s","TKIP");
            }
            
            if (strstr(auth,"none") > 0)
            {
                sprintf(aptable->entry[i].authmode,"%s","Disable");
                sprintf(aptable->entry[i].encryption,"%s","OPEN");
            }
            else if (strstr(auth,"WPA PSK") > 0)
            {
                sprintf(aptable->entry[i].authmode,"%s","WPA-PSK");
            }
            else if (strstr(auth,"WPA2 PSK") > 0)
            {
                sprintf(aptable->entry[i].authmode,"%s","WPA2-PSK");
            }
            else if (strstr(auth,"WPA2 PSK") > 0)
            {
                sprintf(aptable->entry[i].authmode,"%s","WPA2-PSK");
            }
            else if (strstr(auth,"mixed WPA/WPA2 PSK") > 0)
            {
                sprintf(aptable->entry[i].authmode,"%s","WPAPSKWPA2PSK");
            }
            else if (strstr(auth,"WEP Open/Shared") > 0)
            {
                sprintf(aptable->entry[i].authmode,"%s","OPENWEP");
            }
            
            i++;
        }
    }
    
    pclose(pp);
    aptable->Num = i;
    return 0;
}

int getDhcpCliList(struct DhcpInfo *dhcpinfo,int *num)
{
	FILE *fp;
	int i = 0;
	char buffer[256] = {0};
	struct dhcpOfferedAddr {
		unsigned char hostname[16];
		unsigned char mac[18];
		unsigned char ip[18];
		unsigned char expires[16];
	} lease={0};
	struct in_addr addr;
	unsigned long expires;
	unsigned d, h, m;
    //printf("enter %s\n",__func__);
	//system("killall -q -USR1 udhcpd");

	fp = fopen("/var/dhcp.leases", "r");
	if (NULL == fp)
		return -1;

	while (feof(fp) == 0) {
	    memset(buffer,0,256);
	    memset(&lease,0,sizeof(lease));
        fgets(buffer,256,fp);
        if (strlen(buffer) == 0)
        {
            break;
        }
        sscanf(buffer,"%s %s %s %s %*s",lease.expires,lease.mac,lease.ip,lease.hostname);
        //printf("lease.mac=%x\n",lease.mac);
		sprintf(dhcpinfo[i].hostname, "%s", lease.hostname);
		sprintf(dhcpinfo[i].mac,"%s",lease.mac);
		//addr.s_addr = lease.ip;
		expires = atol(lease.expires);
		sprintf(dhcpinfo[i].ip, "%s",lease.ip);
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


int setWirelessReapter(int status,char *ssid,char *bssid,char *channel,char *auth,char *encry,char *key)
{
    char cmd[128] = {0};


    system("uci -q set wireless.SSID4=wifi-iface");

    if (status == 0 )
    {
        //sprintf(cmd,"/sbin/uci set wireless.SSID4.disabled=1");
        //system(cmd);
        //sprintf(cmd,"/sbin/uci delete wireless.SSID4");
        //system(cmd);
        //sprintf(cmd,"/sbin/uci commit wireless");
        //system(cmd);
        //sprintf(cmd,"/usr/bin/env -i /sbin/reloadnetwork.sh network");
        //system(cmd);
        setConnectMode(CONNECT_WIRED_MODE);
        return 0;
    }
    //sprintf(cmd,"/sbin/uci delete network.wan.ifname");
    //system(cmd);
    //sprintf(cmd,"/sbin/uci set network.wan.type=bridge");
    //system(cmd);
    //sprintf(cmd,"/sbin/uci commit network");
    //system(cmd);

    sprintf(cmd,"/sbin/uci set wireless.radio0.channel=%s",channel);
    system(cmd);

    sprintf(cmd,"/sbin/uci set wireless.SSID4=wifi-iface");
    system(cmd);
    sprintf(cmd,"/sbin/uci set wireless.SSID4.device=radio0");
    system(cmd);
    sprintf(cmd,"/sbin/uci set wireless.SSID4.mode=sta");	
    system(cmd);
    sprintf(cmd,"/sbin/uci set wireless.SSID4.ifname='SSID4'");	
    system(cmd);
    sprintf(cmd,"/sbin/uci set wireless.SSID4.bssid=%s",bssid);//
    system(cmd);
    sprintf(cmd,"/sbin/uci set wireless.SSID4.ssid=%s",ssid);//
    system(cmd);
    sprintf(cmd,"/sbin/uci set wireless.SSID4.network=wan");
    system(cmd);

    /*
       接口定义的加密类型
            WPAPSKWPA2PSK      
            WPA-PSK
            WPA2-PSK
            OPENWEP
            Disable
       */
    memset(cmd,0,128);
    if (strcmp(auth,"WPAPSKWPA2PSK") == 0)
        sprintf(cmd,"/sbin/uci set wireless.SSID4.encryption=psk2");
    else if (strcmp(auth,"WPA-PSK") == 0)
        sprintf(cmd,"/sbin/uci set wireless.SSID4.encryption=psk");
    else if (strcmp(auth,"WPA2-PSK") == 0)
        sprintf(cmd,"/sbin/uci set wireless.SSID4.encryption=psk2");
    else if (strcmp(auth,"OPENWEP") == 0)
        sprintf(cmd,"/sbin/uci set wireless.SSID4.encryption=wep");
    else if (strcmp(auth,"Disable") == 0)
        sprintf(cmd,"/sbin/uci set wireless.SSID4.encryption=none");

    system(cmd);
    
    if (strlen(key) > 0)
    {
        sprintf(cmd,"/sbin/uci set wireless.SSID4.key=%s",key);
        system(cmd);
    }
    sprintf(cmd,"/sbin/uci set wireless.SSID4.disabled=0");
    system(cmd);
    sprintf(cmd,"/sbin/uci commit wireless");
    system(cmd);
    //sprintf(cmd,"/usr/bin/env -i /sbin/restart_firewall.sh");
    //system(cmd);
    //memset(cmd,0,128);

    //sprintf(cmd,"/usr/bin/env -i /sbin/reloadnetwork.sh network");
    system(cmd);
    setConnectMode(CONNECT_WIRELESS_MODE);
    return 0;
}


int getWirelessRepeaterInfo(int *status,char *ssid,char *bssid,char *channel,
                            char *auth,char *encry,char *key)
{
    FILE *fp;
    char tmpbuffer[32];
    int mode = 0;
    mode = getConnectMode();
    if (mode == CONNECT_WIRED_MODE)
    {
        *status = 1;
    }
    else if (mode == CONNECT_WIRELESS_MODE)
    {
        *status = 0;
    }
    else
    {
        *status = 1;
    }

    if (*status == 1)
    {
        return 0;
    }


    fp=popen("uci -q get wireless.SSID4.ssid","r");
    if (fp == NULL)
    {
        log("open pipe failed\n");
        return -1;
    }
    
    fgets(tmpbuffer,32,fp);
    sscanf(tmpbuffer,"%[^\n]",ssid);
    pclose(fp);

    fp=popen("/sbin/getssidbssid.sh SSID4","r");
    if (fp == NULL)
    {
        log("open pipe failed\n");
        return -1;
    }
    fgets(tmpbuffer,32,fp);
    sscanf(tmpbuffer,"%[^\n]",bssid);
    pclose(fp);

    fp=popen("getssidchannel.sh SSID4","r");
    if (fp == NULL)
    {
        log("open pipe failed\n");
        return -1;
    }
    fgets(tmpbuffer,32,fp);
    sscanf(tmpbuffer,"%[^\n]",channel);
    pclose(fp);

    get_ssid_authmode(STA_SSID,auth);
    strcpy(encry,"AES");
    /*
    fp=popen("uci -q get wireless.SSID4.key","r");
    if (fp == NULL)
    {
        log("open pipe failed\n");
        return -1;
    }
    fgets(tmpbuffer,32,fp);
    sscanf(tmpbuffer,"%[^\n]",key);
    pclose(fp);
    */
    get_ssid_password(STA_SSID,key);
    
    return 0;
}

void  systemupgrade(char *imagename,char *md5,char *version,char *needclear)
{
    char cmd[128] = {0};
    sprintf(cmd, "/sbin/upgrade.sh %s %s %s %s &", imagename,version,md5,needclear);
    system(cmd);
}


