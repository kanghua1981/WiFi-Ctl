

/*本文件为服务端程序*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h> 
#include <sys/types.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include "pthread.h"
#include "xmldoc.h"
#include "ixml.h"
#include "route.h"

int main_socket = -1;
#define SERVER_PORT      9991
#define DATA_PORT      9992

#define E_OK                    0
#define E_ROUTER_SYSTEM         0xF0000001
#define E_ROUTER_PARAM          0xF0000002
#define E_ROUTER_PARAM_VALUE    0xF0000003
#define E_ROUTER_XML_PARSE      0xF0000004

#define MAX_CLIENT  10

//struct xmldoc {};
/*需要增加信号量*/
typedef struct tcp_client_s
{
    int r_thread_fd;
    int client_sfd;
    int active;
}tcp_client;

FILE *logfile;


tcp_client client_pool[MAX_CLIENT];
tcp_client *find_client()
{
    int i = 0;
    for (i = 0 ;i < MAX_CLIENT;i++)
    {
        if (client_pool[i].active == 0)
            return &(client_pool[i]);
    }
    return NULL;
}


int SendMsg(int sockfd,char * sendBuff);

void MacUptoLow(char *macaddr)
{
    int i;
    char *tmp  = macaddr;
    while (*tmp != 0 && i < 12)
	{
		if (*tmp >= 'A' && *tmp <= 'Z')
		{
			macaddr[i] = tolower(*tmp);
		}
		
		i++;
		tmp++;
	}
}




/**************************************************************
 * 发送消息，如果出错返回-1，否则返回发送的字符长度
 * sockfd：socket标识，sendBuff：发送的字符串
 * *********************************************************/
int  SendMsg(int sockfd,char * sendBuff)
{
    int  sendSize=0;
    int totalsize = strlen(sendBuff);
    while (sendSize < totalsize)
    {
       int size = 0;
       size = send(sockfd,sendBuff + sendSize,totalsize - sendSize,0);
       if(size <= 0)
       {
           printf("Send msg error!\n");
           return -1;
       }
       else
       { 
           printf("send %d\n",size);
           sendSize += size;
       }
    }
    return sendSize;
}



int CreateServerSocket(int port)
{
    struct sockaddr_in sin_addr;
    int opt = 1;
    main_socket = socket(AF_INET,SOCK_STREAM,0);
    if ( -1 == main_socket )
    {
        printf("Create socket error\n");
        perror("");
        return -1;
    }
    setsockopt(main_socket,SOL_SOCKET,SO_REUSEADDR,(const void *)&opt,sizeof(opt));
    memset(&sin_addr,0,sizeof(sin_addr));
    
    sin_addr.sin_family = AF_INET;
    sin_addr.sin_addr.s_addr = INADDR_ANY;
    sin_addr.sin_port = htons(port);

    if (-1 == bind(main_socket,(struct sockaddr*)&sin_addr,sizeof(sin_addr)))
    {
    	 perror("");
    	 return -1;
    }
    
     if (-1 == listen(main_socket, 10) )
    {
        printf("Server Listen Failed!"); 
        return -1;
    }
    
    return 0;
}

void CloseServerSocket()
{
    if (main_socket != -1)
        close(main_socket);
} 


typedef struct cmd_process_s{
    char *cmd;
    int (*process)(int socket,void *args);
}cmd_process;


typedef struct xmlinfo_s{
    struct xmldoc *doc;
    struct xmlelement *root_node;
    struct xmldoc *result;
}xmlinfo;

/*
<root>
　　<OP>WIFIBASIC</OP>
　　<Method>SET</Method>
　　<WiFiBasic>
　　<Channel>0~12</Channel>
　　<Strenth>0~100</Strenth>
　　</WiFiBasic>
</root>
*/
int build_result(struct xmldoc *result,int errno)
{
    struct xmlelement *node;
    struct xmlelement *result_root_node;
    struct xmlelement *root_node = find_element_in_doc(result, "root");
	if (root_node == NULL)
	{
	    root_node = xmldoc_new_topelement(result,"root",NULL);
		//return 0;
    }
    node = xmlelement_new(result,"Result");
    xmlelement_add_element(result,root_node,node);
    if (errno == E_OK)
        xmlelement_add_text(result,node,"OK");
    else
    {
        xmlelement_add_text(result,node,"ERROR");
        //node = xmlelement_new(result,"ErrCode");
        //xmlelement_add_element(result,root_node,node);
        add_value_element_int(result,root_node,"ErrCode",errno);
    }
    return 0;
}

int wifibasic(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmlelement *root_node = info->root_node;
    struct xmlelement *wifi_node;
    char * channel,*strenth,*strCountry;
    
    wifi_node = find_element_in_element(root_node, "WiFiBasic");
    if (wifi_node == NULL)
		return -1;

    value_node = find_element_in_element(wifi_node, "Channel");
    if (value_node) 
    {
        channel = get_node_value(value_node);
    }
    else
    {
        return E_ROUTER_PARAM;
    }
        
    value_node = find_element_in_element(wifi_node, "Strenth");
    if (value_node) 
        strenth = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

	value_node = find_element_in_element(wifi_node, "Country");
    if (value_node) 
        strCountry = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    set_wifi_basic(channel,strenth,strCountry);

    return 0;
}


/*
    <root>
　　<OP>SETSSID</OP>
　　<Method>SET</Method>
　　<WiFi>
    　　<Index>0/1</Index>
    　　<SSID>String</SSID>
    　　<WiFiSwitch>ON/OFF</WiFiSwitch>
    　　<Hide>ON/OFF</Hide>
    　　<ClientNum>0~16</ClientNum>
               <Bandwidth>0~N</Bandwidth>
    　　<AuthMode>WPAPSKWPA2PSK/ WPA-PSK/WPA2-PSK/OPENWEP/Dsiable</AuthMode>
    　　<EncryptionMode>TKIP/AES/TKIPAES</EncryptionMode>
    　　<Password>String</Password> <!--String长度大于8--> 
　　</WiFi>
    </root>
*/
int setssid(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmlelement *root_node = info->root_node;
    struct xmlelement *wifi_node = NULL;
    char *strIndex = 0,*strSsid,*strSwitch,*strHide,*strClientNum,*strBandwidth;
    char *strAuthMode,*strEncryption,*strPassword;
    
    int index = -1;
    
    wifi_node = find_element_in_element(root_node, "WiFi");
    if (wifi_node == NULL)
		return -1;
    value_node = find_element_in_element(wifi_node, "Index");
    if (value_node)
        strIndex = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    index = atoi(strIndex);
    if (index != 0 && index != 1)
        return E_ROUTER_PARAM_VALUE;
    
    
    value_node = find_element_in_element(wifi_node, "SSID");
    if (value_node)
        strSsid = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    value_node = find_element_in_element(wifi_node, "WiFiSwitch");
    if (value_node)
        strSwitch = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    value_node = find_element_in_element(wifi_node, "Hide");
    if (value_node)
        strHide = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;


    value_node = find_element_in_element(wifi_node, "ClientNum");
    if (value_node)
        strClientNum = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;


    value_node = find_element_in_element(wifi_node, "Bandwidth");
    if (value_node)
        strBandwidth = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    value_node = find_element_in_element(wifi_node, "AuthMode");
    if (value_node)
        strAuthMode = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    if (strncmp(strAuthMode,"Disable",7) !=0 )
    {
        //printf("strAuthMode=%s\n",strAuthMode);
        value_node = find_element_in_element(wifi_node, "EncryptionMode");
        if (value_node)
            strEncryption = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;
            
        value_node = find_element_in_element(wifi_node, "Password");
        if (value_node)
            strPassword = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;
    }
    else
    {
        strAuthMode = "OPEN";
        strEncryption = "NONE";
        strPassword = "";
    }

    set_ssid(index,strSsid,strSwitch,strHide,strAuthMode,strEncryption,strPassword,strClientNum);
    return 0;
}

/*
<root>
　　<OP>SETUSER</OP>
　　<Method>SET</Method>
　　<User>
　　<UserName>String</UserName>
　　<Password>String</Password>
　　</User>
</root>
*/
int setuser(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmlelement *root_node = info->root_node;
    struct xmlelement *user_node = NULL; 
    char * admuser,*admpass;
    user_node = find_element_in_element(root_node, "User");
    if (user_node == NULL)
		return -1;
		
    value_node = find_element_in_element(user_node, "UserName");
    if (value_node)
        admuser = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    value_node = find_element_in_element(user_node, "Password");
    if (value_node)
        admpass = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    set_admin(admuser,admpass);
    return 0;
}

/*
<root>
　　<OP>RESTOREDEFAULT</OP>
　　<Method>SET</Method>
　　<RestoreDefault>ON/OFF</RestoreDefault>
</root>

*/
int restoredefault(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmlelement *root_node = info->root_node;
    char *strdefault = NULL;
    value_node = find_element_in_element(root_node, "RestoreDefault");
    
    if (value_node)
        strdefault = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;
    if (strcmp(strdefault,"ON")==0)
        set_default();

    return 0;
}
/*
<root>
　　<OP>SETWAN</OP>
　　<Method>SET</Method>
　　<WAN>
    　　<Mode>PPPoE/DHCP/STATIC</Mode>
    　　<PPPoE>
        　　<User>user</User>
        　　<Password>password</Password>
    　　</PPPoE>
    　　<STATIC>
        　　<IP></IP>
        　　<Netmask></Netmask>
        　　<Gateway></Gateway>
        　　<PremaryDNS></PremaryDNS>
        　　<SecondaryDNS></SecondaryDNS>
    　　</STATIC>
    　　<MAC>000102030405</MAC>
　　</WAN>
</root>
*/
int setwan(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmlelement *root_node = info->root_node;
    struct xmlelement *wan_node;
    struct xmlelement *static_node;
    struct xmlelement *pppoe_node;
	struct xmlelement *bridge_node;
    char *strmode;
    char *strIP,*strNetmask,*strGateway,*strDNS1,*strDNS2;
    char *strUser,*strPwd,*strdomain;
    wan_node = find_element_in_element(root_node, "WAN");
    if (wan_node == NULL)
		return -1;
		
    value_node = find_element_in_element(wan_node, "Mode");
    if (value_node)
        strmode = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    
    if (strncmp(strmode,"DHCP",4) == 0)
    {
        set_dhcpmode();
    }
    else if (strncmp(strmode,"STATIC",6) == 0)
    {
        static_node = find_element_in_element(wan_node, "STATIC");
        if (static_node == NULL)
		    return E_ROUTER_PARAM;

        value_node = find_element_in_element(static_node, "IP");
        if (value_node)
            strIP = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;

        value_node = find_element_in_element(static_node, "Netmask");
        if (value_node)
            strNetmask = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;

        value_node = find_element_in_element(static_node, "Gateway");
        if (value_node)
            strGateway = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;

        value_node = find_element_in_element(static_node, "PremaryDNS");
        if (value_node)
            strDNS1 = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;

        value_node = find_element_in_element(static_node, "SecondaryDNS");
        if (value_node)
            strDNS2 = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;
        set_staticmode(strIP,strNetmask,strGateway,strDNS1,strDNS2);
    }
    else if (strncmp(strmode,"PPPoE",5) == 0)
    {
        pppoe_node = find_element_in_element(wan_node, "PPPoE");
        if (pppoe_node == NULL)
		    return E_ROUTER_PARAM;
		    
        value_node = find_element_in_element(pppoe_node, "User");
        if (value_node)
            strUser = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;

        value_node = find_element_in_element(pppoe_node, "Password");
        if (value_node)
            strPwd = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;

        set_pppoe(strUser,strPwd);
    }
	else if (strncmp(strmode,"BRIDGE",6) == 0)
	{
		bridge_node = find_element_in_element(wan_node, "BRIDGE");
        if (bridge_node == NULL)
		    return E_ROUTER_PARAM;
		value_node = find_element_in_element(bridge_node, "Domain");
        if (value_node)
            strdomain = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;
		set_wan_bridge(strdomain);
	}
    else
    {
        return E_ROUTER_PARAM;
    }
    return 0;
}


/*
<root>
　　<OP>RESTARTNETWORK</OP>
　　<Method>SET</Method>
　　<RestartNetwork>ON/OFF</RestartNetwork>
</root>
*/
int restartnetwork(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmlelement *root_node = info->root_node;
    char *strrestartnetwork = NULL;
    value_node = find_element_in_element(root_node, "RestartNetwork");
    
    if (value_node)
        strrestartnetwork = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;
    if (strcmp(strrestartnetwork,"ON")==0)
        restart_network();

    return 0;
}


int reloadnetwork(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    
    reload_network();

    return 0;
}


/*
<root>
　　<OP>REBOOT</OP>
　　<Method>SET</Method>
</root>
*/
int reboot(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmlelement *root_node = info->root_node;
    sysreboot();

    return 0;
}
/*
<root>
　　<OP>WPS</OP>
　　<Method>SET</Method>
</root>
*/

int setwps(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmlelement *root_node = info->root_node;
    syswps();

    return 0;
}
/**
***
<root>
    <OP>SETSTATICDHCP</OP>
    <Method>SET</Method>
    <CMD>add/del/modify/clear</CMD>
    <Number>3</Number>
    <STATICLIST>
    <STATICMAC>
    <MAC>xx:xx:xx:xx:xx:xx</MAC>
    <IP>xxx.xxx.xxx.xxx</IP>
    </STATICMAC>
    <STATICMAC>
    <MAC>xx:xx:xx:xx:xx:xx</MAC>
    <IP>xxx.xxx.xxx.xxx</IP>
    </STATICMAC>
    <STATICMAC>
    <MAC>xx:xx:xx:xx:xx:xx</MAC>
    <IP>xxx.xxx.xxx.xxx</IP>
    </STATICMAC>
    </STATICLIST>
</root>
 *
 * */

int setstaticdhcp(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
	int ret = 0;
    struct xmlelement *value_node = NULL;
	struct xmlelement *cmd_node;
    struct xmldoc *doc = info->doc;;
    struct xmlelement *root_node = info->root_node;
    struct xmlelement *staticlist;
    struct xmlelement *client_node = NULL;
    IXML_NodeList *clients_node = NULL;
	char *cmd = NULL;
    char *mac,*ip,*strnum;
    int num = 0,i=0;
    //printf("begin static dhcp\n");
	value_node = find_element_in_element(root_node, "CMD");
    if (value_node)
        cmd = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

	if (strncmp(cmd,"clear",5) == 0)
	{
		ret = clear_static_dhcp();
        return ret;
	}

	value_node = find_element_in_element(root_node, "Number");
    if (value_node)
        strnum = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    staticlist = find_element_in_element(root_node,"STATICLIST");
    if (staticlist == NULL)
        return E_ROUTER_PARAM;
   
    num = atoi(strnum);
    if (num == 0)
        return E_ROUTER_PARAM;

    //printf("kkk=%d\n",num);
    clients_node = ixmlDocument_getElementsByTagName(staticlist,"STATICMAC");
    if (clients_node == NULL)
        return E_ROUTER_PARAM; 

    for (i = 0;i < num;i++)
    {
       client_node = ixmlNodeList_item(clients_node,i);
       if (client_node) 
       {
           value_node = find_element_in_element(client_node, "MAC");
           if (value_node)
           {
               mac = get_node_value(value_node);
           }  

           value_node = find_element_in_element(client_node, "IP");
           if (value_node)
           {
               ip = get_node_value(value_node);
           }  

           if (strncmp(cmd,"add",3) == 0)
           {
               ret = add_static_dhcp(mac,ip);
           }
           else if (strncmp(cmd,"del",3) == 0)
           {
               ret = del_static_dhcp(mac,ip);
           }
           else if (strncmp(cmd,"modify",6) == 0)
           {
               ret = modify_static_dhcp(mac,ip);
           }
       } 
    }
    apply_static_dhcp();
	return ret;

}

int setscanstart(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmlelement *root_node = info->root_node;
    system("iwinfo SSID1 scan > /tmp/tmpscan");
    return 0;
}

/*
<root>
<OP>SETWIFIREPEATER</OP>
<Method>SET</Method>
<Enable>1/0<Enable>
<SSID>xxx</SSID>
<BSSID>xx:xx:xx:xx:xx:xx</BSSID>
<Encryption>psk2-mixed</Encryption>
<Channel>xx</Channel>
<Key>xxxxxxxx</Key>
</root>
int setWirelessReapter(char *ssid,char *bssid,char *channel,char *encry,char *key)

*/
int wifirepeater(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmlelement *root_node = info->root_node;
    char *enable = NULL;
    char *ssid = NULL;
    char *bssid=NULL;
    char *authmode = NULL;
    char *encryption = NULL;
    char *channel = NULL;
    char *key = NULL;
    
    value_node = find_element_in_element(root_node, "Enable");
    if (value_node)
        enable = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    if (strcmp(enable,"1") == 0)
    {
        value_node = find_element_in_element(root_node, "SSID");
        if (value_node)
            ssid = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;

        value_node = find_element_in_element(root_node, "BSSID");
        if (value_node)
            bssid = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;

        value_node = find_element_in_element(root_node, "AuthMode");
        if (value_node)
            authmode = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;

        value_node = find_element_in_element(root_node, "Encryption");
        if (value_node)
            encryption = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;

        value_node = find_element_in_element(root_node, "Channel");
        if (value_node)
            channel = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;

        value_node = find_element_in_element(root_node, "Key");
        if (value_node)
            key = get_node_value(value_node);
        else
            return E_ROUTER_PARAM;

        if (strncmp(authmode,"WPAPSKWPA2PSK",13)==0)
        {
            strcpy(authmode,"WPA2-PSK");
        }
        if (strncmp(encryption,"TKIPAES",7) ==0)
        {
            strcpy(encryption,"AES");
        }

        setWirelessReapter(1,ssid,bssid,channel,authmode,encryption,key);
    }
    else
    {
        setWirelessReapter(0,NULL,NULL,NULL,NULL,NULL,NULL);
    }

}

/*
<root>
<OP>UPGRADE</OP>
<Method>SET</Method>
<UpgradeImage>xxxxx</UpgradeImage>
</root>
*/
int upgrade(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    int ret = 0;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmlelement *root_node = info->root_node;
    char *image = NULL;
    char *md5 = NULL;
    char *version = NULL;
    char *needclear = NULL;
#if 0
    value_node = find_element_in_element(root_node, "UpgradeImage");
    if (value_node)
        image = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    value_node = find_element_in_element(root_node, "MD5");
    if (value_node)
        md5 = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

#endif 
    value_node = find_element_in_element(root_node, "Version");
    if (value_node)
        version = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    value_node = find_element_in_element(root_node, "NeedClear");
    if (value_node)
        needclear = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    ret = systemupgrade(image,md5,version,needclear);

    return ret;
}
/*
 *<root>
  <OP>SETWIRELESSCODE</OP>
  <Method>SET</Method>
  <Timeout>1~120s</Timeout>
  </root>
 *
 * */
int setwirelesscode(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmlelement *root_node = info->root_node;
    char *timeout = NULL;
    int tout;
    value_node = find_element_in_element(root_node, "Timeout");
    if (value_node)
        timeout = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    tout = atoi(timeout);

    broadcast_wirelesscode(tout);

    return 0;

}
int kickmac(int socket,void *args)
{
    
    return 0;
}
cmd_process setcmd[] = {
    {"WIFIBASIC",wifibasic},
    {"SETSSID",setssid},
    {"SETUSER",setuser},
    {"RESTOREDEFAULT",restoredefault},
    {"SETWAN",setwan},
    {"RESTARTNETWORK",restartnetwork},
    {"RELOADNETWORK",reloadnetwork},
    {"SETWIFIREPEATER",wifirepeater},
    {"UPGRADE",upgrade},
    {"KICKMAC",kickmac},
    {"REBOOT",reboot},
    {"SETSCANSTART",setscanstart},
    {"WPS",setwps},
    {"SETSTATICDHCP",setstaticdhcp},
    {"SETWIRELESSCODE",setwirelesscode},
};

/*
req:
<root>
　　<OP>GETWIFIBASIC</OP>
　　<Method>GET</Method>
</root>

response:
<root>
　　<OP>GETWIFIBASIC</OP>
　　<Method>RESPONSE</Method>
　　<WiFiBasic>
　　<Channel>0~12</Channel>
　　<Strenth>0~100</Strenth>
　　</WiFiBasic>
</root>

*/
int getwifibasic(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmldoc *result_doc = info->result;
    struct xmlelement *root_node = info->root_node;
    struct xmlelement *result_root_node = NULL;
    struct xmlelement *node = NULL;
    struct xmlelement *wifi_node = NULL;
    char strChannel[4]={0},strStrenth[6]={0},strCountry[8]={0};
    
	get_wifi_basic(strChannel,strStrenth,strCountry);
		
    result_root_node = xmldoc_new_topelement(result_doc,"root",NULL);

    node = xmlelement_new(result_doc,"OP");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"GETWIFIBASIC");

    node = xmlelement_new(result_doc,"Method");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"RESPONSE");

    wifi_node = xmlelement_new(result_doc,"WiFiBasic");
    if (wifi_node == NULL)
        return E_ROUTER_SYSTEM;
    xmlelement_add_element(result_doc,result_root_node,wifi_node);
    
    node = xmlelement_new(result_doc,"Channel");
    if (node == NULL)
        return E_ROUTER_SYSTEM;
        
    xmlelement_add_element(result_doc,wifi_node,node);
    xmlelement_add_text(result_doc,node,strChannel);

    node = xmlelement_new(result_doc,"Strenth");
    if (node == NULL)
        return E_ROUTER_SYSTEM;
    xmlelement_add_element(result_doc,wifi_node,node);
    xmlelement_add_text(result_doc,node,strStrenth);

	node = xmlelement_new(result_doc,"Country");
    if (node == NULL)
        return E_ROUTER_SYSTEM;
    xmlelement_add_element(result_doc,wifi_node,node);
    xmlelement_add_text(result_doc,node,strCountry);
    return 0;
}

/*
<root>
　　<OP>GETSSID</OP>
　　<Method>GET</Method>
　　<SSID>
　　<Index>0/1</Index>
　　</SSID>
</root>

<root>
　　<OP>GETSSID</OP>
　　<Method>RESPONSE</Method>
　　<WiFi>
    　　<Index>0/1</Index>
    　　<SSID>String</SSID>
    　　<WiFiSwitch>ON/OFF</WiFiSwitch>
    　　<Hide>ON/OFF</Hide>
         　<ClientNum>0~16</ClientNum>
               <Bandwidth>0~N</Bandwidth>
    　　<AuthMode>WPAPSKWPA2PSK/WPA-PSK/WPA2-PSK/OPENWEP/Disable</AuthMode>
　　    <EncryptionMode>TKIP/AES/TKIPAES</EncryptionMode>
　　    <Password>String</Password> <!--String长度大于8--> 
　　</WiFi>
</root>
*/
int getssid(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmldoc *result_doc = info->result;
    struct xmlelement *root_node = info->root_node;
    struct xmlelement *result_root_node = NULL;
    struct xmlelement *node = NULL;
    struct xmlelement *ssid_node = NULL;
    struct xmlelement *wifi_node = NULL;
    char *strIndex,*ifname;
    char strssid[32],strSwitch[4]= {0},strHide[16]= {0};
    char *strClientNum="0",*strBandwidth="20M",strAuth[16]={0};
    char strEncry[16]= {0},strPwd[16]={0};
    int index;

    /*获取index*/
    ssid_node = find_element_in_element(root_node, "SSID");
    if (ssid_node == NULL)
		return E_ROUTER_SYSTEM;

    value_node = find_element_in_element(ssid_node, "Index");
    if (value_node)
        strIndex = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;
    
    index = atoi(strIndex);
    if (index != 0 && index != 1)
    {
        return E_ROUTER_PARAM_VALUE;
    }
    
    get_ssid(index,strssid);
    get_ssid_authmode(index,strAuth);
    get_ssid_hide(index,strHide);

    get_wifi_status(strSwitch);
    //strHide = LFF(RT2860_NVRAM, "HideSSID");
    
    result_root_node = xmldoc_new_topelement(result_doc,"root",NULL);

    node = xmlelement_new(result_doc,"OP");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"GETSSID");

    node = xmlelement_new(result_doc,"Method");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"RESPONSE");

    wifi_node = xmlelement_new(result_doc,"WiFi");
    xmlelement_add_element(result_doc,result_root_node,wifi_node);

    node = xmlelement_new(result_doc,"Index");
    xmlelement_add_element(result_doc,wifi_node,node);
    xmlelement_add_text(result_doc,node,strIndex);


    node = xmlelement_new(result_doc,"SSID");
    xmlelement_add_element(result_doc,wifi_node,node);
    xmlelement_add_text(result_doc,node,strssid);

    node = xmlelement_new(result_doc,"WiFiSwitch");
    xmlelement_add_element(result_doc,wifi_node,node);
    xmlelement_add_text(result_doc,node,strSwitch);

    node = xmlelement_new(result_doc,"Hide");
    xmlelement_add_element(result_doc,wifi_node,node);
    xmlelement_add_text(result_doc,node,strHide);

    node = xmlelement_new(result_doc,"ClientNum");
    xmlelement_add_element(result_doc,wifi_node,node);
    xmlelement_add_text(result_doc,node,strClientNum);

    node = xmlelement_new(result_doc,"Bandwidth");
    xmlelement_add_element(result_doc,wifi_node,node);
    xmlelement_add_text(result_doc,node,strBandwidth);

    node = xmlelement_new(result_doc,"AuthMode");
    xmlelement_add_element(result_doc,wifi_node,node);
    xmlelement_add_text(result_doc,node,strAuth);

    if (strcmp(strAuth,"Disable") != 0)
    {
        get_ssid_encry(index,strEncry);
        node = xmlelement_new(result_doc,"EncryptionMode");
        xmlelement_add_element(result_doc,wifi_node,node);
        xmlelement_add_text(result_doc,node,strEncry);

        get_ssid_password(index,strPwd);
        node = xmlelement_new(result_doc,"Password");
        xmlelement_add_element(result_doc,wifi_node,node);
        xmlelement_add_text(result_doc,node,strPwd);
    }
    
    return 0;
}

/*
<root>
　　<OP>GETUSER</OP>
　　<Method>GET</Method>
</root>

<root>
　　<OP>SETUSER</OP>
　　<Method>RESPONSE</Method>
　　<User>
    　　<UserName>String</UserName>
    　　<Password>String</Password>
　　</User>
</root>
*/
int getuser(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmldoc *result_doc = info->result;
    struct xmlelement *result_root_node = NULL;
    struct xmlelement *node = NULL;
    struct xmlelement *user_node = NULL;
    char admuser[16],admpass[16];

    get_admin(admuser,admpass);

    result_root_node = xmldoc_new_topelement(result_doc,"root",NULL);

    node = xmlelement_new(result_doc,"OP");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"GETUSER");

    node = xmlelement_new(result_doc,"Method");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"RESPONSE");

    user_node = xmlelement_new(result_doc,"User");
    xmlelement_add_element(result_doc,result_root_node,user_node);

    node = xmlelement_new(result_doc,"UserName");
    xmlelement_add_element(result_doc,user_node,node);
    xmlelement_add_text(result_doc,node,admuser);

    node = xmlelement_new(result_doc,"Password");
    xmlelement_add_element(result_doc,user_node,node);
    xmlelement_add_text(result_doc,node,admpass);

    return 0;
}
/*
<root>
　　<OP>GETWAN</OP>
　　<Method>GET</Method>
</root>

<root>
　　<OP>GETWAN</OP>
　　<Method>RESPONSE</Method>
　　<WAN>
    　　<Mode>PPPoE/DHCP/STATIC</Mode>
    　　<PPPoE>
        　　<User>user</User>
        　　<Password>password</Password>
    　　</PPPoE>
    　　<STATIC>
        　　<IP>xxx.xxx.xxx.xxx</IP>
        　　<Netmask>xxx.xxx.xxx.xxx</Netmask>
        　　<Gateway>xxx.xxx.xxx.xxx</Gateway>
        　　<PremaryDNS>xxx.xxx.xxx.xxx</PremaryDNS>
        　　<SecondaryDNS>xxx.xxx.xxx.xxx</SecondaryDNS>
    　　</STATIC>
    　　<MAC>000102030405</MAC>
　　</WAN>
</root>
*/
int getwan(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmldoc *result_doc = info->result;
    struct xmlelement *result_root_node = NULL;
    struct xmlelement *node = NULL;
    struct xmlelement *wan_node = NULL;
    char mode[12];
    get_wan_mode(mode);
    
    result_root_node = xmldoc_new_topelement(result_doc,"root",NULL);

    node = xmlelement_new(result_doc,"OP");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"GETWAN");

    node = xmlelement_new(result_doc,"Method");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"RESPONSE");

    wan_node = xmlelement_new(result_doc,"WAN");
    xmlelement_add_element(result_doc,result_root_node,wan_node);

    node = xmlelement_new(result_doc,"Mode");
    xmlelement_add_element(result_doc,wan_node,node);
    xmlelement_add_text(result_doc,node,mode);

    if (strncmp(mode,"DHCP",4) == 0)
        return 0;
    else if (strncmp(mode,"PPPOE",5) == 0)
    {
        struct xmlelement *pppoe_node = NULL;
        char user[32]= {0},pwd[32]={0};
        pppoe_node = xmlelement_new(result_doc,"PPPOE");
        xmlelement_add_element(result_doc,wan_node,pppoe_node);

        get_pppoe_info(user,pwd);
        node = xmlelement_new(result_doc,"User");
        xmlelement_add_element(result_doc,pppoe_node,node);
        xmlelement_add_text(result_doc,node,user);

        node = xmlelement_new(result_doc,"Password");
        xmlelement_add_element(result_doc,pppoe_node,node);
        xmlelement_add_text(result_doc,node,pwd);
    }
    else if (strncmp(mode,"STATIC",6) == 0)
    {
        struct xmlelement *static_node = NULL;
        char IP[17]={0},netmask[17]={0},gateway[17]= {0};
        char dns1[17]= {0},dns2[17] = {0};
        static_node = xmlelement_new(result_doc,"STATIC");
        xmlelement_add_element(result_doc,wan_node,static_node);

        get_static_info(IP,netmask,gateway,dns1,dns2);
        
        node = xmlelement_new(result_doc,"IP");
        xmlelement_add_element(result_doc,static_node,node);
        xmlelement_add_text(result_doc,node,IP);

        node = xmlelement_new(result_doc,"Netmask");
        xmlelement_add_element(result_doc,static_node,node);
        xmlelement_add_text(result_doc,node,netmask);

        node = xmlelement_new(result_doc,"Gateway");
        xmlelement_add_element(result_doc,static_node,node);
        xmlelement_add_text(result_doc,node,gateway);

        node = xmlelement_new(result_doc,"PremaryDNS");
        xmlelement_add_element(result_doc,static_node,node);
        xmlelement_add_text(result_doc,node,dns1);

        node = xmlelement_new(result_doc,"SecondaryDNS");
        xmlelement_add_element(result_doc,static_node,node);
        xmlelement_add_text(result_doc,node,dns2);

    }
    else if (strncmp(mode,"BRIDGE",6) == 0)
    {
		struct xmlelement *bridge_node = NULL;
		bridge_node = xmlelement_new(result_doc,"BRIDGE");
        xmlelement_add_element(result_doc,wan_node,bridge_node);
		node = xmlelement_new(result_doc,"Domain");
        xmlelement_add_element(result_doc,bridge_node,node);
        xmlelement_add_text(result_doc,node,"lan");
	}
    return 0;
}


/************************************************
<root>
　　<OP>GETWIFICLIENT</OP>
　　<Method>GET</Method>
　　<WiFiClient>
　　    <Index>0/1</Index>
　　</WiFiClient>
</root>

<root>
　　<Result>OK</Result>
　　<OP>GETWIFICLIENT</OP>
　　<Method>RESPONSE</Method>
　　<Number>3</Number>
　　<ClientList>
    　　<WiFiClient>
        　　<MAC>xxxxxxxxxxxx</MAC>
        　　<BW>20/40</BW>
    　　</WiFiClient>
    　　<WiFiClient>
        　　<MAC>xxxxxxxxxxxx</MAC>
        　　<BW>20/40</BW>
    　　</WiFiClient>
        　　<WiFiClient>
        　　<MAC>xxxxxxxxxxxx</MAC>
        　　<BW>20/40</BW>
    　　</WiFiClient>
　　</ClientList>
</root>
****************************************************/
int getwificlient(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmldoc *result_doc = info->result;
    struct xmlelement *root_node = info->root_node;
    struct xmlelement *result_root_node = NULL;
    struct xmlelement *node = NULL;
    struct xmlelement *clientlist_node = NULL;
    struct xmlelement *client_node = NULL;
    char *strIndex = NULL;
    char *ifname = NULL;
    int index;
    int ret;
    int i;
    char strnum[8] = {0};
    char macaddr[14] = {0};
    char bw[8] = {0};
    STA_MAC_TABLE table;
    
    /*获取index*/
    client_node = find_element_in_element(root_node, "WiFiClient");
    if (client_node == NULL)
		return E_ROUTER_SYSTEM;

    value_node = find_element_in_element(client_node, "Index");
    if (value_node)
        strIndex = get_node_value(value_node);
    else
        return E_ROUTER_PARAM;

    index = atoi(strIndex);
    if (index == 0 )
    {
        ifname = "SSID1";
    }
    else if (index == 1)
    {
        ifname = "SSID2";
    }
    else
        return E_ROUTER_PARAM_VALUE;

    ret = getWlanStaInfo(ifname,&table);
    if (ret < 0)
    {
        return E_ROUTER_SYSTEM;
    }

    /*创建root节点*/
    result_root_node = xmldoc_new_topelement(result_doc,"root",NULL);

    node = xmlelement_new(result_doc,"OP");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"GETWIFICLIENT");

    node = xmlelement_new(result_doc,"Method");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"RESPONSE");

    sprintf(strnum,"%d",table.Num);
    node = xmlelement_new(result_doc,"Number");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,strnum);

    clientlist_node = xmlelement_new(result_doc,"ClientList");
    if (clientlist_node == NULL)
		return E_ROUTER_SYSTEM;
    
    xmlelement_add_element(result_doc,result_root_node,clientlist_node);
	
    for (i = 0; i < table.Num; i++) {
		//sprintf(macaddr, "%02X:%02X:%02X:%02X:%02X:%02X",
		//		table.entry[i].mac[0], table.entry[i].mac[1],
		//		table.entry[i].mac[2], table.entry[i].mac[3],
		//		table.entry[i].mac[4], table.entry[i].mac[5]);
		
		//sprintf(bw, "%s",(table.Entry[i].TxRate.field.BW == 0)? "20M":"40M");
        
		client_node = xmlelement_new(result_doc,"WiFiClient");
		if (client_node == NULL)
		    return E_ROUTER_SYSTEM;

        xmlelement_add_element(result_doc,clientlist_node,client_node);
		
		node = xmlelement_new(result_doc,"MAC");
        xmlelement_add_element(result_doc,client_node,node);
        xmlelement_add_text(result_doc,node,table.entry[i].mac);

        node = xmlelement_new(result_doc,"IP");
        xmlelement_add_element(result_doc,client_node,node);
        xmlelement_add_text(result_doc,node,(const char*)table.entry[i].ip);

        node = xmlelement_new(result_doc,"Rxrate");
        xmlelement_add_element(result_doc,client_node,node);
        xmlelement_add_text(result_doc,node,(const char*)table.entry[i].rxrate);

        node = xmlelement_new(result_doc,"BW");
        xmlelement_add_element(result_doc,client_node,node);
        xmlelement_add_text(result_doc,node,(const char*)table.entry[i].bw);

		node = xmlelement_new(result_doc,"RSSI");
        xmlelement_add_element(result_doc,client_node,node);
        xmlelement_add_text(result_doc,node,(const char*)table.entry[i].rssi);
	}
    
    return 0;
}

int getdhcpstatic(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmldoc *result_doc = info->result;
    struct xmlelement *root_node = info->root_node;
    struct xmlelement *result_root_node = NULL;
    struct xmlelement *node = NULL;
    struct xmlelement *dhcplist_node = NULL;
    struct xmlelement *client_node = NULL;
    char strnum[8];
    STATIC_DHCP_TABLE dhcptable;
    int i;
    
    /*创建root节点*/
    result_root_node = xmldoc_new_topelement(result_doc,"root",NULL);

    node = xmlelement_new(result_doc,"OP");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"GETDHCPSTATIC");

    node = xmlelement_new(result_doc,"Method");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"RESPONSE");

    list_static_dhcp(&dhcptable); 
    sprintf(strnum,"%d",dhcptable.num);

    
    node = xmlelement_new(result_doc,"Number");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,strnum);

    dhcplist_node = xmlelement_new(result_doc,"StaticList");
    xmlelement_add_element(result_doc,result_root_node,dhcplist_node);
    for (i = 0 ;i < dhcptable.num;i++)
    {
        client_node = xmlelement_new(result_doc,"StaticClient");
        xmlelement_add_element(result_doc,dhcplist_node,client_node);

        node = xmlelement_new(result_doc,"MAC");
        xmlelement_add_element(result_doc,client_node,node);
        xmlelement_add_text(result_doc,node,dhcptable.entry[i].mac);

        node = xmlelement_new(result_doc,"IP");
        xmlelement_add_element(result_doc,client_node,node);
        xmlelement_add_text(result_doc,node,dhcptable.entry[i].ip);
        
    }
    
    return 0;
}

/*****************************************************
<root>
　　<OP>GETDHCPCLIENT</OP>
　　<Method>GET</Method>
</root>

<root>
　　<Result>OK</Result>
　　<OP>GETDHCPCLIENT</OP>
　　<Method>RESPONSE</Method>
　　<Number>3</Number>
　　<DHCPList>
    　　<DhcpClient>
        　　<HostName>xxxxx</HostName>
        　　<MAC>xxxxxxxx</MAC>
        　　<IP>xx.xx.xx.xx</IP>
        　　<LeaseTime>xx:xx:xx</LeaseTime>
    　　</DhcpClient>
    　　<DhcpClient>
        　　<HostName>xxxxx</HostName>
        　　<MAC>xxxxxxxx</MAC>
        　　<IP>xx.xx.xx.xx</IP>
        　　<LeaseTime>xx:xx:xx</LeaseTime>
    　　</DhcpClient>
    　　<DhcpClient>
        　　<HostName>xxxxx</HostName>
        　　<MAC>xxxxxxxx</MAC>
        　　<IP>xx.xx.xx.xx</IP>
        　　<LeaseTime>xx:xx:xx</LeaseTime>
    　　</DhcpClient>
　　</DHCPList>
</root>
********************************************************/
int getdhcpclient(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    //struct xmldoc *doc = info->doc;;
    struct xmldoc *result_doc = info->result;
    struct xmlelement *root_node = info->root_node;
    struct xmlelement *result_root_node = NULL;
    struct xmlelement *node = NULL;
    struct xmlelement *dhcplist_node = NULL;
    struct xmlelement *client_node = NULL;
    char strnum[8];
    struct DhcpInfo lease[50]={0};
    int num;
    int i;
    
    /*创建root节点*/
    result_root_node = xmldoc_new_topelement(result_doc,"root",NULL);

    node = xmlelement_new(result_doc,"OP");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"GETDHCPCLIENT");

    node = xmlelement_new(result_doc,"Method");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"RESPONSE");

    getDhcpCliList(lease,&num);
    sprintf(strnum,"%d",num);

    
    node = xmlelement_new(result_doc,"Number");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,strnum);

    dhcplist_node = xmlelement_new(result_doc,"DHCPList");
    xmlelement_add_element(result_doc,result_root_node,dhcplist_node);
    for (i = 0 ;i < num;i++)
    {
        client_node = xmlelement_new(result_doc,"DhcpClient");
        xmlelement_add_element(result_doc,dhcplist_node,client_node);

        node = xmlelement_new(result_doc,"HostName");
        xmlelement_add_element(result_doc,client_node,node);
        xmlelement_add_text(result_doc,node,lease[i].hostname);

        node = xmlelement_new(result_doc,"MAC");
        xmlelement_add_element(result_doc,client_node,node);
        xmlelement_add_text(result_doc,node,lease[i].mac);

        node = xmlelement_new(result_doc,"IP");
        xmlelement_add_element(result_doc,client_node,node);
        xmlelement_add_text(result_doc,node,lease[i].ip);

        node = xmlelement_new(result_doc,"LeaseTime");
        xmlelement_add_element(result_doc,client_node,node);
        xmlelement_add_text(result_doc,node,lease[i].expires);
        
    }
    
    return 0;
}



/*****************************************************
<root>
　　<OP>GETSYSINFO</OP>
　　<Method>GET</Method>
</root>

<root>
　　<Result>OK</Result>
　　<OP>GETSYSINFO</OP>
　　<Method>RESPONSE</Method>
　　<SYS>
　　    <NetType>Gateway</NetType>
　　</SYS>
　　<WIFI>
    　　<SSID>xxxx</SSID>
    　　<Channel>1~12</Channel>
    　　<RadioSwitch>ON/OFF</RadioSwitch>
　　</WIFI>
　　<WAN>
    　　<Connect>Connected/Disconnected</Connect>
    　　<IP>xxx.xxx.xxx.xxx</IP>
    　　<Netmask>255.255.xxx.xxx</Netmask>
    　　<DefaultGateway>xxx.xxx.xxx.xxx</DefaultGateway>
    　　<PremaryDNS>xxx.xxx.xxx.xxx</PremaryDNS>
    　　<SecondaryDNS>xxx.xxx.xxx.xxx</SecondaryDNS>
    　　<MAC>xxxxxxxxxxxx</MAC>
　　</WAN>
</root>
*******************************************************/
int getsysinfo(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmldoc *result_doc = info->result;
    struct xmlelement *root_node = info->root_node;
    struct xmlelement *result_root_node = NULL;
    struct xmlelement *node = NULL;
    struct xmlelement *sys_node = NULL;
    struct xmlelement *wifi_node = NULL;
    struct xmlelement *wan_node = NULL;
    char strssid[32] = {0},strchannel[4] = {0},strSwitch[5],*strCon;
    char strIP[18]= {0},strNetMask[18]={0};
    char strGateway[18]= {0};
    char dns1[17],dns2[17];
    char strMac[18]={0};
    char strVer[32] = {0};
    int conmode = 0;
    int ret = 0;
    int num = 0;
    
    //printf("begin wan\n");
    get_version(strVer);
    //printf("begin ssid\n");
    get_ssid(0,strssid);
    //printf("begin channel\n");
    get_channel(strchannel);
    //printf("begin dns\n");
    get_Dns(dns1,dns2,&num);
    if (num == 0)
    {
        strcpy(dns1,"0.0.0.0");
        strcpy(dns2,"0.0.0.0");
    }
    else if (num == 1)
    {
        strcpy(dns2,"0.0.0.0");
    }
    //printf("begin getIfNetmask\n");
    ret = getIfNetmask(get_wan_ifname(),strNetMask);
    if (ret != 0)
    {
        strcpy(strNetMask,"0.0.0.0");
        //return E_ROUTER_SYSTEM;
    }
    //printf("begin getWanIp\n");
    ret = getWanIp(strIP);
    if (ret != 0)
    {
        strcpy(strIP,"0.0.0.0");
        //return E_ROUTER_SYSTEM;
    }
    ret = getWanMac(strMac);
    if (ret != 0)
    {
        strcpy(strMac,"00:00:00:00:00:00");
        //return E_ROUTER_SYSTEM;
    }
    //printf("begin getWanGateway\n");
    ret = getWanGateway(strGateway);
    if (ret != 0)
    {
        strcpy(strGateway,"0.0.0.0");
        //return E_ROUTER_SYSTEM;
    }
    //printf("begin get_wifi_status\n");
    get_wifi_status(strSwitch);
    //strSwitch = nvram_bufget(RT2860_NVRAM, "WiFiOff");

    //strCon = nvram_bufget(RT2860_NVRAM, "");
    if (strcmp(strIP,"0.0.0.0")==0 || strcmp(strGateway,"0.0.0.0")==0 )
    {
        strCon = "disconnected";
    }
    else
    {
        strCon = "connected";
    }

    conmode = getConnectMode();
    
    /*创建root节点*/
    result_root_node = xmldoc_new_topelement(result_doc,"root",NULL);

    node = xmlelement_new(result_doc,"OP");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"GETSYSINFO");

    node = xmlelement_new(result_doc,"Method");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"RESPONSE");
        
    sys_node = xmlelement_new(result_doc,"SYS");
    xmlelement_add_element(result_doc,result_root_node,sys_node);

    node = xmlelement_new(result_doc,"Version");
    xmlelement_add_element(result_doc,sys_node,node);
    xmlelement_add_text(result_doc,node,strVer);
    
    /*add by kanghua:增加无线或者有线连接模式信息*/
    node = xmlelement_new(result_doc,"ConMode");
    xmlelement_add_element(result_doc,sys_node,node);
    if (conmode == CONNECT_WIRED_MODE)
        xmlelement_add_text(result_doc,node,"wired");
    else if (conmode == CONNECT_WIRELESS_MODE)
        xmlelement_add_text(result_doc,node,"wireless");
    else
        xmlelement_add_text(result_doc,node,"");


    wifi_node = xmlelement_new(result_doc,"WIFI");
    xmlelement_add_element(result_doc,result_root_node,wifi_node);

    node = xmlelement_new(result_doc,"SSID");
    xmlelement_add_element(result_doc,wifi_node,node);
    xmlelement_add_text(result_doc,node,strssid);

    node = xmlelement_new(result_doc,"Channel");
    xmlelement_add_element(result_doc,wifi_node,node);
    xmlelement_add_text(result_doc,node,strchannel);

    node = xmlelement_new(result_doc,"RadioSwitch");
    xmlelement_add_element(result_doc,wifi_node,node);
    xmlelement_add_text(result_doc,node,strSwitch);

    wan_node = xmlelement_new(result_doc,"WAN");
    xmlelement_add_element(result_doc,result_root_node,wan_node);

    node = xmlelement_new(result_doc,"Connect");
    xmlelement_add_element(result_doc,wan_node,node);
    xmlelement_add_text(result_doc,node,strCon);

    node = xmlelement_new(result_doc,"IP");
    xmlelement_add_element(result_doc,wan_node,node);
    xmlelement_add_text(result_doc,node,strIP);

    node = xmlelement_new(result_doc,"Netmask");
    xmlelement_add_element(result_doc,wan_node,node);
    xmlelement_add_text(result_doc,node,strNetMask);


    node = xmlelement_new(result_doc,"DefaultGateway");
    xmlelement_add_element(result_doc,wan_node,node);
    xmlelement_add_text(result_doc,node,strGateway);

    node = xmlelement_new(result_doc,"PremaryDNS");
    xmlelement_add_element(result_doc,wan_node,node);
    xmlelement_add_text(result_doc,node,dns1);

    node = xmlelement_new(result_doc,"SecondaryDNS");
    xmlelement_add_element(result_doc,wan_node,node);
    xmlelement_add_text(result_doc,node,dns2);

    node = xmlelement_new(result_doc,"MAC");
    xmlelement_add_element(result_doc,wan_node,node);
    xmlelement_add_text(result_doc,node,strMac);
    
    return 0;
}
int getwanlink(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmldoc *result_doc = info->result;
    struct xmlelement *root_node = info->root_node;
    struct xmlelement *result_root_node = NULL;
    struct xmlelement *node = NULL;
    char link[2] = {0};
    /*创建root节点*/
    result_root_node = xmldoc_new_topelement(result_doc,"root",NULL);

    node = xmlelement_new(result_doc,"OP");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"GETWANLINK");

    node = xmlelement_new(result_doc,"Method");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"RESPONSE");

    node = xmlelement_new(result_doc,"Link");
    xmlelement_add_element(result_doc,result_root_node,node);
    _getwanlink(link);
    xmlelement_add_text(result_doc,node,link);
    return 0;
}

/*

<root>
<Result>OK</Result>
<OP>SCANWIFI</OP>
<Method>RESPONSE</Method>
<Number>3</Number>
<WiFiList>
<WiFiPoint>
<SSID>xxx</SSID>
<BSSID>xx:xx:xx:xx:xx:xx</BSSID>
<Encryption>xx.xx.xx.xx</Encryption>
<Channel>1~14</Channel>
<Signal>-xx</Signal>
</WiFiPoint>
<WiFiPoint>
<SSID>xxx</SSID>
<BSSID>xx:xx:xx:xx:xx:xx</BSSID>
<Encryption>xx.xx.xx.xx</Encryption>
<Channel>1~14</Channel>
<Signal>-xx</Signal>
</WiFiPoint>
<WiFiPoint>
<SSID>xxx</SSID>
<BSSID>xx:xx:xx:xx:xx:xx</BSSID>
<Encryption>xx.xx.xx.xx</Encryption>
<Channel>1~14</Channel>
<Signal>-xx</Signal>
</WiFiPoint>
</WiFiList>
</root>

*/
int scanwifi(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmldoc *result_doc = info->result;
    struct xmlelement *root_node = info->root_node;
    struct xmlelement *result_root_node = NULL;
    struct xmlelement *node = NULL;
    struct xmlelement *aplist_node = NULL;
    struct xmlelement *ap_node = NULL;
    SCAN_AP_TABLE aptable;
    char strnum[4] = {0};
    int i;
    int ret;
    /*创建root节点*/
    result_root_node = xmldoc_new_topelement(result_doc,"root",NULL);
    node = xmlelement_new(result_doc,"OP");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"SCANWIFI");

    node = xmlelement_new(result_doc,"Method");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"RESPONSE");
    memset(&aptable,0,sizeof(aptable));
    ret = getScanApInfo(&aptable);
    if (ret == 0)
    {
        sprintf(strnum,"%d",aptable.Num);
        node = xmlelement_new(result_doc,"Number");
        xmlelement_add_element(result_doc,result_root_node,node);
        xmlelement_add_text(result_doc,node,strnum);

        aplist_node = xmlelement_new(result_doc,"WiFiList");
        if (aplist_node == NULL)
    		return E_ROUTER_SYSTEM;

        xmlelement_add_element(result_doc,result_root_node,aplist_node);

        //printf("aptable.Num=%d\n",aptable.Num);
        for (i = 0; i < aptable.Num; i++) {
            ap_node = xmlelement_new(result_doc,"WiFiPoint");
    		if (ap_node == NULL)
    		    return E_ROUTER_SYSTEM;
    		xmlelement_add_element(result_doc,aplist_node,ap_node);
		
    		node = xmlelement_new(result_doc,"SSID");
            xmlelement_add_element(result_doc,ap_node,node);
            xmlelement_add_text(result_doc,node,aptable.entry[i].ssid);

            node = xmlelement_new(result_doc,"BSSID");
            xmlelement_add_element(result_doc,ap_node,node);
            xmlelement_add_text(result_doc,node,aptable.entry[i].bssid);

            node = xmlelement_new(result_doc,"AuthMode");
            xmlelement_add_element(result_doc,ap_node,node);
            xmlelement_add_text(result_doc,node,aptable.entry[i].authmode);
            
            node = xmlelement_new(result_doc,"Encryption");
            xmlelement_add_element(result_doc,ap_node,node);
            xmlelement_add_text(result_doc,node,aptable.entry[i].encryption);

            node = xmlelement_new(result_doc,"Channel");
            xmlelement_add_element(result_doc,ap_node,node);
            xmlelement_add_text(result_doc,node,aptable.entry[i].channel);

            node = xmlelement_new(result_doc,"Signal");
            xmlelement_add_element(result_doc,ap_node,node);
            xmlelement_add_text(result_doc,node,aptable.entry[i].signal);
    	    
        }
    }
    else
        return ret;
    return 0;
}

/*
<root>
<Result>OK</Result>
<OP>GETWIFIREPEATER</OP>
<Method>RESPONSE</Method>
<Status>1/0</Status>
<SSID>xxx</SSID>
<BSSID>xx:xx:xx:xx:xx:xx</BSSID>
<Encryption>psk2-mixed</Encryption>
<Channel>xx</Channel>
<Key>xxxxxxxx</Key>
</root>


*/
int getwifirepeater(int socket,void *args)
{
    xmlinfo *info = (xmlinfo*)args;
    struct xmlelement *value_node = NULL;
    struct xmldoc *doc = info->doc;;
    struct xmldoc *result_doc = info->result;
    struct xmlelement *root_node = info->root_node;
    struct xmlelement *result_root_node = NULL;
    struct xmlelement *node = NULL;
    int ret = 0;
    int status = 0;
    char ssid[32] = {0};
    char bssid[24] = {0};
    char channel[8] = {0};
    char authmode[32] = {0};
    char encry[16] = {0};
    char key[32] = {0};
    //printf("enter getwifirepeater\n");
    result_root_node = xmldoc_new_topelement(result_doc,"root",NULL);
    node = xmlelement_new(result_doc,"OP");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"GETWIFIREPEATER");

    node = xmlelement_new(result_doc,"Method");
    xmlelement_add_element(result_doc,result_root_node,node);
    xmlelement_add_text(result_doc,node,"RESPONSE");
    //printf("enter getWirelessRepeaterInfo\n");
    ret = getWirelessRepeaterInfo(&status,ssid,bssid,channel,authmode,encry,key);

    if (ret == 0)
    {
        /*获取的status状态和实际状态相反*/
        if (status == 0)
        {
            node = xmlelement_new(result_doc,"Status");
            xmlelement_add_element(result_doc,result_root_node,node);
            xmlelement_add_text(result_doc,node,"1");

            node = xmlelement_new(result_doc,"SSID");
            xmlelement_add_element(result_doc,result_root_node,node);
            xmlelement_add_text(result_doc,node,ssid);

            node = xmlelement_new(result_doc,"BSSID");
            xmlelement_add_element(result_doc,result_root_node,node);
            xmlelement_add_text(result_doc,node,bssid);

            node = xmlelement_new(result_doc,"AuthMode");
            xmlelement_add_element(result_doc,result_root_node,node);
            xmlelement_add_text(result_doc,node,authmode);

            node = xmlelement_new(result_doc,"Encryption");
            xmlelement_add_element(result_doc,result_root_node,node);
            xmlelement_add_text(result_doc,node,encry);

            node = xmlelement_new(result_doc,"Channel");
            xmlelement_add_element(result_doc,result_root_node,node);
            xmlelement_add_text(result_doc,node,channel);

            node = xmlelement_new(result_doc,"Key");
            xmlelement_add_element(result_doc,result_root_node,node);
            xmlelement_add_text(result_doc,node,key);
            
        }
        else
        {
            node = xmlelement_new(result_doc,"Status");
            xmlelement_add_element(result_doc,result_root_node,node);
            xmlelement_add_text(result_doc,node,"0");
        }
    }
    else
        return ret;
    
    return 0;
}


cmd_process getcmd[] = {
    {"GETWIFIBASIC",getwifibasic},
    {"GETSSID",getssid},
    {"GETUSER",getuser},
    {"GETWAN",getwan},
    {"GETWIFICLIENT",getwificlient},
    {"GETDHCPCLIENT",getdhcpclient},
    {"GETDHCPSTATIC",getdhcpstatic},
    {"GETSYSINFO",getsysinfo},
    {"GETWANLINK",getwanlink},
    {"SCANWIFI",scanwifi},
    {"GETWIFIREPEATER",getwifirepeater}
};


int response(int socket,struct xmldoc *result)
{
    char *sendbuff;
    sendbuff = xmldoc_tostring(result);
    if (sendbuff == NULL)
    {
        return -1;
    }
    SendMsg(socket,sendbuff);
    return 0;
}

/*simple process command*/
int process_cmd(int socket,char *buffer)
{
    char *xmlfilename = NULL;
    char *op = NULL,*method = NULL;
    int i;
    int errcode = 0;
    xmlinfo info;
    struct xmldoc *doc = NULL;
    struct xmlelement *value_node = NULL;
    doc = xmldoc_parsexml(buffer);
    if (doc == NULL)
    {
        printf("xml parse failed\n");
        memset(&info,0,sizeof(xmlinfo));
        info.result = xmldoc_new();
        
        xmldoc_new_topelement(info.result,"root",NULL);
        
        build_result(info.result,E_ROUTER_XML_PARSE);
        
        response(socket,info.result);
        
        xmldoc_free(info.result);
        
		return 0;
	}
	

    struct xmlelement *root_node = find_element_in_doc(doc, "root");
	if (root_node == NULL)
	{
        memset(&info,0,sizeof(xmlinfo));
        info.result = xmldoc_new();
        
        xmldoc_new_topelement(info.result,"root",NULL);
        
        build_result(info.result,E_ROUTER_XML_PARSE);
        
        response(socket,info.result);

        xmldoc_free(info.result);
	    xmldoc_free(doc);
		return 0;
	}

	value_node = find_element_in_element(root_node, "OP");
	if (value_node) op = get_node_value(value_node);

	value_node = find_element_in_element(root_node, "Method");
	if (value_node) method = get_node_value(value_node);

    //printf("op=%s method=%s\n",op,method);
    info.doc = doc;
    info.root_node = root_node;
    info.result = xmldoc_new();
    if (strncmp(method,"SET",3) == 0)
    {
        for (i = 0;i < sizeof(setcmd)/sizeof(cmd_process);i++)
        {
            if (0 == strncmp(op,setcmd[i].cmd,strlen(op)))
            {
                errcode = setcmd[i].process(socket,&info);
                break;
            }
        }
    }
    else
    {
        for (i = 0;i < sizeof(getcmd)/sizeof(cmd_process) ;i++)
        {
            if (0 == strncmp(op,getcmd[i].cmd,strlen(op)))
            {
                errcode = getcmd[i].process(socket,&info);
                break;
            }
        }
    }
    
    build_result(info.result,errcode);

    response(socket,info.result);
    xmldoc_free(info.result);
    xmldoc_free(info.doc);
    
    return 0;
}

#if 0
int process_packet(int socket)
{
    char recvchar;
    char recvbuff[1024] = {0};
    char *pbuf = recvbuff;
    char linebuf[128] = {0};
    int ret = 0;
    char *pline = linebuf;
    //从服务器接收数据到buffer中
    while (1)
    {
        int length = recv(socket,&recvchar,1,0);
        if(length <= 0)
        {
            printf("Recieve Data From Client Failed!\n");
            perror("");
            //close(socket); 
            //exit(0);
            break;
        }
        
        *pbuf = recvchar;
        pbuf++;
        
        //printf("%c",recvchar);
        *pline = recvchar;
        pline++;
        //if (recvchar == '\n')
        //{
            /*
            if (strncmp(linebuf,"</root>",7)==0)
            {
                ret=process_cmd(socket,recvbuff);
                if (ret != 0)
                {
                    break;
                }
            }
            */
            //memset(linebuf,0,128);
            //pline = linebuf;
        //}
        
        if (recvchar == '\0')
        {
            ret = process_cmd(socket,recvbuff);
            if (ret != 0)
            {
                break;
            }
            memset(recvbuff,0,1024);
            pbuf= recvbuff;
        }
            //break;
    }

}
#else
char recvbuff[4096] = {0};
int process_packet(int socket)
{
    int ret = 0;
    int length = 0;
    int total = 0;
    //struct timeval tv_out;
    //tv_out.tv_sec = 10;
   // tv_out.tv_usec = 0;
    //从服务器接收数据到buffer中
    //如果30s收不到数据就超时返回
    //setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
    //从服务器接收数据到buffer中
    while (1)
    {
        total = 0;
        while(1)
        {
            length = recv(socket,recvbuff+total,2048-total,0);
            if(length <= 0)
            {
                printf("Recieve Data From Client Failed!\n");
                perror("");
                //close(socket); 
                //exit(0);
                return 0;
            }
            total += length; 
            if (total > 2048)
            {
                printf("Buffer is larger then 2048\n");
                return 0;
            }
            if(strstr(recvbuff,"</root>")>0)
            {
                printf("total=%d\n",total);
                break; 
            }
        }
#if 1
        ret = process_cmd(socket,recvbuff);
        if (ret != 0)
        {
            break;
        }
#else
        {
            pid_t fpid; 
            fpid=fork();
            if (fpid < 0)   
                printf("error in fork!");   
            else if (fpid == 0) {  
                close(main_socket);
                process_cmd(socket,recvbuff);                             
                close(socket);
                exit(0);
            }   
        }
#endif
        memset(recvbuff,0,2048);
    }
    return 0;

}
#endif




void *tcp_client_thread(void *param)
{
    tcp_client *pclient = (tcp_client*)param;

    process_packet(pclient->client_sfd);
    close(pclient->client_sfd);
    pclient->active = 0;
    return NULL;
}

int write_firmware(char *buffer,int size)
{
    int fd = 0;
    int ret = 0;
    int written = 0;
    fd = open("/tmp/upgradefirmware.bin",O_CREAT | O_RDWR);
    if (fd < 0)
    {
        printf("open upgradefirmware failed\n");
        return -1;
    }
    written = 0;
    while(written < size)
    {
        ret = write(fd,buffer+written,size-written);
        if (ret < 0)
        {
            close(fd);
            printf("write upgradefirmware failed\n");
            return -1;
        }
         written +=ret;
    }
    close(fd);
    return 0;
}

#define MAX_DBUFF_SIZE (10*1024*1024)
int DataProcess()
{
    struct sockaddr_in client_addr;
    socklen_t slength = sizeof(client_addr);
    int size = 0;
    int ret = 0;
    int total = 0;
    char *recvbuff = NULL;
    if (CreateServerSocket(DATA_PORT) < 0)
       return -1;
    while(1)
    {
        int length = 0;
        int new_server_socket = accept(main_socket,(struct sockaddr*)&client_addr,&slength);
        if ( new_server_socket < 0)
        {
            printf("Server Accept Failed!\n");
            perror("");
            break;
        }

        length = recv(new_server_socket,&size,4,0);
        if(length <= 0)
        {
            printf("Recieve Size From Client Failed!\n");
            perror("");
            close(new_server_socket);
            continue;
        }

        if (size <  MAX_DBUFF_SIZE)
        {
            recvbuff = malloc(size);
            if (recvbuff == NULL)
                return -1;
        }
        else
        {
            printf("size of firmware is too big %x\n",size);
            SendMsg(new_server_socket,"size of firmware is too big");
            close(new_server_socket);
            continue;
        }

        while(total < size)
        {
            int length = 0;
            length = recv(new_server_socket,recvbuff+total,size-total,0);
            if(length <= 0)
            {
                printf("Recieve Firmware From Client Failed!\n");
                perror("");
                close(new_server_socket);
                break;
            }
            total += length; 
        }
        if (total == size)
        {
            ret = write_firmware(recvbuff,total);
            if (ret != 0)
                SendMsg(new_server_socket,"write disk failed");
            else
                SendMsg(new_server_socket,"upload ok");
        }
        free(recvbuff);
        close(new_server_socket);
    }
    CloseServerSocket();
    return 0;
}

int main(int argc,char **argv)
{
    int ret;
    tcp_client *pclient;
#if 1
    pid_t pid;
    pid=fork();
    if(pid<0)  
    {
        return -1;
    }
    if (pid == 0)
    {
        DataProcess();
        exit(1);
    }
#endif

    if (CreateServerSocket(SERVER_PORT) < 0)
        return -1;
    signal(SIGCHLD, SIG_IGN);
    logfile=fopen("/tmp/connectlog","w+");
    while (1) //服务器端要一直运行
    {
        pid_t fpid; 
        //定义客户端的socket地址结构client_addr
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);

        //接受一个到server_socket代表的socket的一个连接
        //如果没有连接请求,就等待到有连接请求--这是accept函数的特性
        //accept函数返回一个新的socket,这个socket(new_server_socket)用于同连接到的客户的通信
        //new_server_socket代表了服务器和客户端之间的一个通信通道
        //accept函数把连接到的客户端信息填写到客户端的socket地址结构client_addr中
        int new_server_socket = accept(main_socket,(struct sockaddr*)&client_addr,&length);
        if ( new_server_socket < 0)
        {
            printf("Server Accept Failed!\n");
            perror("");
            break;
        }

#if 0
        /* maybe casue memory leak*/
        pclient = find_client();
        if (pclient == NULL)
        {
            close(new_server_socket);
            continue;
        }

        pclient->active = 1;
        pclient->client_sfd = new_server_socket;

        ret = pthread_create(&pclient->r_thread_fd,NULL,tcp_client_thread,pclient);
        if (0 != ret)
        {
            printf("Create client thread failed\n");
            perror("");
            return -1;  
        }
        pthread_join(pclient->r_thread_fd,NULL);
#else
        pclient = &client_pool[0];
        
        pclient->active = 1;
        pclient->client_sfd = new_server_socket;

        fpid=fork();
        if (fpid < 0)   
             printf("error in fork!");   
        else if (fpid == 0) {  
             close(main_socket);
             tcp_client_thread((void*)pclient);                             
             exit(0);
        }  
        close(new_server_socket);
#endif
        //process_packet(new_server_socket);
       // close(new_server_socket);
    }

    fclose(logfile);
     //关闭监听用的socket
    CloseServerSocket();
    return 0;
}


