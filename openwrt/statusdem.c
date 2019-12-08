

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

static int getphylink(const char *ifname)
{
    FILE *pp;
    char cmd[32] = {0};
    char buffer[32] = {0};
    sprintf(cmd,"/sbin/getethlink.sh %s",ifname);
    pp = popen(cmd,"r");
    if (pp == NULL)
        return 0;
    fgets(buffer,8,pp);
   // printf("%s:%s\n",ifname,buffer);
    pclose(pp);
    if (strncmp(buffer,"up",2)==0)
    {
        //printf("getphylink:up\n");
        return 1;
    }
    else
    {
        //printf("getphylink:down\n");
        return 0;
    }
}

static int haslinkup = 0;

static void checkwanstatus()
{
    char wanname[16] = {0};
    int wanmode = 0;
    char mode[16] = {0};
    wanmode = getConnectMode();
    if (wanmode == CONNECT_WIRED_MODE)
    {
        get_wan_mode(mode);
        if (strcmp(mode,"DHCP") == 0)
        {
            get_wan_name(wanname);
            if (getphylink(wanname) == 0 && haslinkup)
            {
                system("killall udhcpc");
                haslinkup = 0;
            }
            else
            {
                haslinkup = 1;
            }
        }
    }
    
}


static int checklinkspeed(int port)
{
    FILE *pp;
    char cmd[64] = {0};
    char buffer[64] = {0};
    sprintf(cmd,"/sbin/getportlinkspeed.sh %d",port);
    pp = popen(cmd,"r");
    if (pp == NULL)
    {
        return 0;
    }
    fgets(buffer,64,pp);
    
   // printf("%s:%s\n",ifname,buffer);
    pclose(pp);
    /*如果没有link状态，说明没有连接，不需要复位网络*/
    if (strlen(buffer) == 0)
    {
        return 1;
    }
    if (strncmp(buffer,"100baseT full-duplex auto",25)==0)
    {
        return 1;
    }
    return 0;
}

int checknetwork()
{
    while(1)
    {
        sleep(5);
        checkwanstatus();
        /*如果不是100M全双工则down up eth0,
                为了规避link问题*/
        if (0 == checklinkspeed(3))
        {
            system("ifconfig eth0 down && ifconfig eth0 up");
        }
    }
}

