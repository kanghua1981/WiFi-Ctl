

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "xmldoc.h"

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

int main(int argc, char **argv)
{
    int fd;
    char buffer[1024]= {0};
    int ret;
    char *xmlfilename = NULL;
    struct xmldoc *doc = NULL;
    struct xmlelement *value_node = NULL;
    char *op,*method,*channel,*strenth;
    if (argc < 2)
    {
        return -1;
    }
    
    xmlfilename = argv[1];
    fd = open(xmlfilename,O_RDWR);
    if (fd < 0)
    {
        printf("open %s failed\n",xmlfilename);
        return -1;
    }
    ret = read(fd,buffer,1024);
    if (ret < 0 )
    {
        printf("read %s failed\n",xmlfilename);
    }
    
    //printf("%s\n",buffer);
    
    doc = xmldoc_parsexml(buffer);
    if (doc == NULL)
    {
        printf("xml parse failed\n");
		return 0;
	}

    struct xmlelement *root_node = find_element_in_doc(doc, "root");
	if (root_node == NULL)
		return 0;

	value_node = find_element_in_element(root_node, "OP");
	if (value_node) op = get_node_value(value_node);

	value_node = find_element_in_element(root_node, "Method");
	if (value_node) method = get_node_value(value_node);

    printf("op=%s method=%s\n",op,method);
    
    struct xmlelement *wifi_node = find_element_in_element(root_node,
							       "WiFiBasic");
	if (wifi_node == NULL)
		return 0;
    
	value_node = find_element_in_element(wifi_node, "Channel");
	if (value_node) channel = get_node_value(value_node);

    value_node = find_element_in_element(wifi_node, "Strenth");
	if (value_node) strenth = get_node_value(value_node);

    printf("channel=%s,strenth=%s\n",channel,strenth);

    close(fd);
    return 0;

}
