




#ifndef _ROUTE_H
#define _ROUTE_H

/* route basic interface */
struct DhcpInfo {
	unsigned char hostname[16];
	unsigned char mac[18];
	unsigned char ip[18];
	unsigned char expires[12];
} ;
typedef struct sta_mac_entry_s{
    unsigned char mac[20];
	unsigned char ip[18];
    unsigned char bw[4];
	unsigned char rssi[4];
	unsigned char rxrate[8];
}STA_MAC_ENTRY;

#define CONNECT_WIRED_MODE         1
#define CONNECT_WIRELESS_MODE      2

typedef struct sta_mac_table_s{
    STA_MAC_ENTRY entry[20];
    int Num;
}STA_MAC_TABLE;

typedef struct ap_info_entry_s{
    char ssid[64];
    char bssid[24];
    char channel[32];
    //char encryption[32];
    char authmode[32];
    char encryption[32];
    char signal[32];
}AP_INFO_ENTRY;

typedef struct scan_ap_table_s{
    int Num;
    AP_INFO_ENTRY entry[100];
}SCAN_AP_TABLE;


typedef struct static_dhcp_entry_s{
    char mac[32];
    char ip[17];
}static_dhcp_entry;

typedef struct static_dhcp_table_s{
    int num;
    static_dhcp_entry entry[100];
}STATIC_DHCP_TABLE;

int set_wifi_basic(char *channel,char *strenth,char *strCountry);
int set_ssid(int index,char *strSsid,char *strSwitch,char *strHide,
                char *strAuthMode,char *strEncryption,char *strPassword,char *strClientNum);
int set_admin(char *admuser,char *admpass);
int set_default();
int set_dhcpmode();
int set_staticmode(char *strIP,char *strNetmask,char*strGateway,char *strDNS1,char *strDNS2);
int set_pppoe(char* strUser,char *strPwd);
int set_wan_bridge(char *domain);

int get_wifi_basic(char *Channel,char *Strenth,char *strCountry);
int get_admin(char *user,char *pwd);
int getDhcpCliList(struct DhcpInfo *dhcpinfo,int *num);
int getIfIp(char *ifname, char *if_addr);
char* getWanIfNamePPP(void);
int getWlanStaInfo(char *ifname,STA_MAC_TABLE *table);

int get_version(char *version);
//int get_wan_mode(char *mode);
int get_pppoe_info(char *user,char *pwd);
int get_static_info(char *ip,char *netmask,char *gateway,char *dns1,char *dns2);
int getWanIp(char *if_addr);
int getIfMac(char *ifname, char *if_hw);
int getWanMac(char *if_mac);
char  *get_wan_ifname();
int getIfNetmask(char *ifname, char *if_net);
int getWanGateway(char *sgw);
int get_ssid(int index,char *ssid);
int get_channel(char *channel);
int get_Dns(char *dns1,char *dns2,int *num);

int get_ssid_authmode(int index,char *auth);
int get_ssid_hide(int index,char *hide);
int get_ssid_encry(int index,char *encry);
int get_ssid_password(int index,char *pwd);
int get_wifi_status(char *status);
void restart_network(void);
void reload_network(void);
void sysreboot(void);
int getScanApInfo(SCAN_AP_TABLE *aptable);
int  systemupgrade(char *imagename,char *md5,char *version,char *needclear);
void syswps();

int apply_static_dhcp();
int list_static_dhcp(STATIC_DHCP_TABLE *dhcptable);
int add_static_dhcp(char *mac,char *ip);
int del_static_dhcp(char *mac,char *ip);
int modify_static_dhcp(char *mac,char *ip);
#endif



