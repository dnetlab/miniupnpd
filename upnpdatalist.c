#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include "upnpredirect.h"
#include "upnpdatalist.h"

#ifdef TOMATO
static struct list_head upnpd_list;
struct list_head *g_upnpd_list = &upnpd_list;

static int protocol_atoi(const char * protocol)
{
	int proto = IPPROTO_TCP;
	if(strcmp(protocol, "UDP") == 0)
		proto = IPPROTO_UDP;
	return proto;
}

static char *protocol_itoa(unsigned int proto)
{
	if (proto == IPPROTO_TCP)
		return ("TCP");
	else
		return ("UDP");
}

void dump_upnp_list(void)
{
	int i = 1;
	struct list_head *pos;
	list_for_each(pos, g_upnpd_list) {
		struct rule_info *new = list_entry(pos,
			struct rule_info, list);
		fprintf(stderr, "%d %d %s %d %s %d %s %d\n",
			i, new->enable, protocol_itoa(new->protocol),
			new->eport, inet_ntoa(new->ipaddr),
			new->iport, new->desc, new->timestamp);
		i++;
	}
}

void destory_upnpd_list(void)
{
	struct list_head *pos, *n;
	struct rule_info *new;

	list_for_each_safe(pos, n, g_upnpd_list){
		//delete node
       	list_del(pos);
       	new = list_entry(pos, struct rule_info, list);		
		upnp_delete_redirection(new->eport, protocol_itoa(new->protocol));
       	//free node
	   	free(new);
	}
	return ;
}

int delete_node_to_upnp_list(unsigned int protocol, unsigned short eport)
{
	struct list_head *pos, *n;
	struct rule_info *new;

	list_for_each_safe(pos, n, g_upnpd_list){
      	new = list_entry(pos, struct rule_info, list);
		if(new->protocol == protocol && new->eport == eport)
		{		
			//delete node
       		list_del(pos);
       		//free node
	   		free(new);
			return 1;
		}
	}
	return 0;
}

struct rule_info *is_node_exist_upnpd_list(struct rule_info info)
{
	struct list_head *pos;
	list_for_each(pos, g_upnpd_list) {
		struct rule_info *new = list_entry(pos,
			struct rule_info, list);
		if (new->ipaddr.s_addr == info.ipaddr.s_addr && new->eport == info.eport &&
			new->iport == info.iport && new->protocol == info.protocol)
			return new;
	}
	return NULL;
}

int add_node_to_upnpd_list(struct rule_info info)
{
	struct rule_info *newNode = (struct rule_info *)malloc(sizeof(struct rule_info));

	if(newNode != NULL)
	{
		newNode->enable = info.enable;
		newNode->eport = info.eport;
		newNode->ipaddr = info.ipaddr;
		newNode->iport = info.iport;
		newNode->protocol = info.protocol;
		newNode->timestamp = info.timestamp;
		memcpy(newNode->desc, info.desc, sizeof(newNode->desc) - 1);

		list_add(&newNode->list, g_upnpd_list);	
	}
	return 0;
}

void add_and_update_upnpd_list_info(
	unsigned int enable, 
	unsigned short eport,
    const char * iaddr,
    unsigned short iport,
    const char * proto,
    const char * desc,
    unsigned int timestamp
)
{
	struct rule_info info, *new = NULL;
	memset(&info, 0x0, sizeof(struct rule_info));

	info.enable = enable;
	info.eport = eport;
	info.ipaddr.s_addr = inet_addr(iaddr);
	info.iport = iport;
	info.protocol = protocol_atoi(proto);

	info.timestamp = timestamp;
	memcpy(info.desc, desc, sizeof(info.desc) - 1);

	new = is_node_exist_upnpd_list(info);
	if(new != NULL)
	{
		new->enable = info.enable;		
		new->timestamp = timestamp;
		memcpy(new->desc, desc, sizeof(info.desc) - 1);
	}
	else
	{
		add_node_to_upnpd_list(info);
	}
}

/* reload_data_from_file()
 * read lease_file and add the rules contained
 */
int reload_data_from_file(const char *pathfile)
{
	FILE * fd;
	unsigned short eport, iport;	
	unsigned int leaseduration;
	unsigned int timestamp;
	unsigned int enable;
	
	char proto[10] = {0}, ipaddr[16] = {0}, desc[64] = {0};
	char line[128] = {0};
	int matchs = 0, r = 0;	
	char * rhost;
	time_t current_time;

	if(!pathfile) 
		return -1;
	
	fd = fopen(pathfile, "r" );
	if (fd == NULL) 
	{
		syslog(LOG_ERR, "could not open lease file: %s", pathfile);
		return -1;
	}

	current_time = time(NULL);
	while(fgets(line, sizeof(line), fd)) 
	{
		syslog(LOG_DEBUG, "parsing lease file line '%s'", line);
		
		matchs = sscanf(line, "%u:%[^:]:%hu:%[^:]:%hu:%u:%[^:]", &enable, proto, &eport, ipaddr, &iport, &timestamp, desc);
				
		if(matchs != 7)
			continue;

		if(timestamp > 0) 
		{
			if(timestamp <= (unsigned int)current_time) 
			{
				syslog(LOG_NOTICE, "already expired lease in lease file");
				continue;
			} 
			else 
			{
				leaseduration = timestamp - current_time;
			}
		} else 
		{
			leaseduration = 0;	/* default value */
		}
		
		rhost = NULL;
		r = upnp_redirect(enable, rhost, eport, ipaddr, iport, proto, desc, leaseduration);
		if(r < 0) 
		{
			syslog(LOG_ERR, "Failed to redirect %hu -> %s:%hu protocol %s",
			       eport, ipaddr, iport, proto);
		}
	}
	fclose(fd);

	return 0;
}

void init_upnpd_list_data(void)
{
	INIT_LIST_HEAD(g_upnpd_list);

	if(access("/etc/upnp", F_OK) == -1)
	{
		mkdir("/etc/upnp", 0755);
	}

	reload_data_from_file(UPNP_DATA_LIST_PATHFILE);
	dump_upnp_list();
}

int write_date_to_file(const char *pathfile)
{
	FILE *fp = NULL;
	
	fp = fopen( pathfile, "w");
	if (fp != NULL) 
	{
		struct list_head *pos;
		struct rule_info *new;
		
		list_for_each(pos, g_upnpd_list){
			new = list_entry(pos, struct rule_info, list);
			fprintf(fp, "%d:%s:%d:%s:%d:%d:%s\n", 
				new->enable, protocol_itoa(new->protocol),
				new->eport, inet_ntoa(new->ipaddr), new->iport,
				new->timestamp, new->desc);
		}
		fclose(fp);
	}
	return 0;
}

void delete_data_from_file(void)
{
	FILE *f;
	char s[128];
	unsigned short eport;
	unsigned short iport;
	unsigned int leaseduration;
	char proto[4];
	char iaddr[32];
	char desc[64];
	char rhost[32];
	int n;

	if ((f = fopen(UPNP_DATA_DEL_PATHTILE, "r")) != NULL)
	{
		s[sizeof(s) - 1] = 0;
		while (fgets(s, sizeof(s) - 1, f))
		{
			if (sscanf(s, "%3s %hu", proto, &eport) == 2)
			{
				if (proto[0] == '*')
				{
					n = upnp_get_portmapping_number_of_entries();
					while (--n >= 0)
					{
						if (upnp_get_redirection_infos_by_index(n, &eport, proto, &iport, iaddr, sizeof(iaddr), desc, sizeof(desc), rhost, sizeof(rhost), &leaseduration) == 0)
						{
							delete_node_to_upnp_list(protocol_atoi(proto), eport);
							upnp_delete_redirection(eport, proto);
						}
					}
					break;
				}
				else
				{
					delete_node_to_upnp_list(protocol_atoi(proto), eport);
					upnp_delete_redirection(eport, proto);
				}
			}
		}
		fclose(f);
	}

}

#endif

