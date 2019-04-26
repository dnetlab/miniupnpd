#ifndef __UPNP_DATA_LIST_H
#define __UPNP_DATA_LIST_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "list.h"

#ifdef TOMATO
struct rule_info{
	struct list_head list;
	unsigned int enable;
	unsigned short eport;
	unsigned short iport;	
	struct in_addr ipaddr;
	unsigned int protocol;
	unsigned int timestamp;
	unsigned char desc[64];
};

#define UPNP_DATA_LIST_PATHFILE		("/etc/upnp/data")
#define UPNP_DATA_SAVE_PATHFILE		("/etc/upnp/save")
#define UPNP_DATA_DEL_PATHTILE		("/etc/upnp/delete")

void destory_upnpd_list(void);
void dump_upnp_list(void);
void init_upnpd_list_data(void);
void add_and_update_upnpd_list_info(unsigned int enable, unsigned short eport,
    const char * iaddr, unsigned short iport, const char * proto, const char * desc, unsigned int timestamp);
int delete_node_to_upnp_list(unsigned int protocol, unsigned short eport);
int write_date_to_file(const char *pathfile);
void delete_data_from_file(void);
#endif
#endif // __UPNP_DATA_LIST_H

