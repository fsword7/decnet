/******************************************************************************
    (c) 2008 Christine Caulfield            christine.caulfield@googlemail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 ******************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netdnet/dn.h>
#include <netdnet/dnetdb.h>
#include "libnetlink.h"

/* Sigh - people keep removing features ... */
#ifndef NDA_RTA
#define NDA_RTA(r)  ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))
#endif

#define IDENT_STRING "DECnet for Linux"

#define NODESTATE_UNKNOWN    -1
#define NODESTATE_ON          0
#define NODESTATE_OFF         1
#define NODESTATE_SHUT        2
#define NODESTATE_RESTRICTED  3
#define NODESTATE_REACHABLE   4
#define NODESTATE_UNREACHABLE 5

typedef int (*neigh_fn_t)(struct sockaddr_nl *who, struct nlmsghdr *n, void *arg);

static struct rtnl_handle talk_rth;
static struct rtnl_handle listen_rth;
static int first_time = 1;
static int verbose;
static unsigned short router_node;

#define MAX_ADJACENT_NODES 1024
static int num_adj_nodes = 0;
static unsigned short adj_node[MAX_ADJACENT_NODES];

static int num_link_nodes = 0;
static struct link_node
{
	unsigned char node, area;
	unsigned int links;
} link_nodes[MAX_ADJACENT_NODES];

static void makeupper(char *s)
{
	int i;
	for (i=0; i<strlen(s); i++) s[i] = toupper(s[i]);
}

static int adjacent_node(struct nodeent *n)
{
    int i;
    unsigned short nodeid = n->n_addr[0] | n->n_addr[1]<<8;

    for (i=0; i<num_adj_nodes; i++)
        if (adj_node[i] == nodeid)
	    return 1;

    return 0;
}

static char *if_index_to_name(int ifindex)
{
    struct ifreq ifr;
    static char buf[64];

    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    ifr.ifr_ifindex = ifindex;

    if (ioctl(sock, SIOCGIFNAME, &ifr) == 0)
    {
	strcpy(buf, ifr.ifr_name);
    }
    else
    {
	sprintf(buf, "if%d", ifindex);
    }

    close(sock);
    return buf;
}


/* Get the router address from /proc */
static unsigned short get_router(void)
{
	char buf[256];
	char var1[32];
	char var2[32];
	char var3[32];
	char var4[32];
	char var5[32];
	char var6[32];
	char var7[32];
	char var8[32];
	char var9[32];
	char var10[32];
	char var11[32];
	unsigned short router = 0;
	FILE *procfile = fopen("/proc/net/decnet_dev", "r");

	if (!procfile)
		return 0;
	while (!feof(procfile))
	{
		fgets(buf, sizeof(buf), procfile);
		if (sscanf(buf, "%s %s %s %s %s %s %s %s %s ethernet %s\n",
			   var1,var2,var3,var4,var5,var6,var7,var8,var9,var11) == 10)
		{
			int area, node;
			sscanf(var11, "%d.%d\n", &area, &node);
			router = area<<10 | node;
			break;
		}
	}
	fclose(procfile);
	dnetlog(LOG_DEBUG, "Router node is %x\n", router);
	return router;
}

static int get_link_count(unsigned char addr1, unsigned char addr2)
{
	int node = addr1 | (addr2<<8 & 0x3);
	int area = addr2 >> 2;
	int i;

	for (i=0; i<num_link_nodes; i++) {
		if (link_nodes[i].area == area &&
		    link_nodes[i].node == node) {
			return link_nodes[i].links;
		}
	}
	return 0;
}

static int send_node(int sock, struct nodeent *n, int exec, char *device, int state)
{
	char buf[1024];
	int ptr = 0;

	makeupper(n->n_name);
	buf[ptr++] = 1; // Here is your data miss
	buf[ptr++] = 0;
	buf[ptr++] = 0; // Status
	buf[ptr++] = 0; // Node information
	buf[ptr++] = n->n_addr[0];
	buf[ptr++] = n->n_addr[1];
	buf[ptr++] = strlen(n->n_name) | (exec?0x80:0);
	memcpy(&buf[ptr], n->n_name, strlen(n->n_name));
	ptr += strlen(n->n_name);

	/* Device */
	if (device) {
		buf[ptr++] = 0x36;  //
		buf[ptr++] = 0x3;   // 822=CIRCUIT

		buf[ptr++] = 0x40;  // ASCII text
		buf[ptr++] = strlen(device);
		strcpy(&buf[ptr], device);
		ptr += strlen(device);
	}

	/* Node State */
	if (state != NODESTATE_UNKNOWN) {
		struct nodeent *rn;
		struct nodeent scratch_n;
		unsigned char scratch_na[2];
		int links;

	        scratch_na[0] = router_node & 0xFF;
	        scratch_na[1] = router_node >>8;

		buf[ptr++] = 0;   // 0=Node state
		buf[ptr++] = 0;

		buf[ptr++] = 0x81; // Data type & length of 'state'
		buf[ptr++] = state;

		/* For reachable nodes show the next node, ie next router
		   it itself */
		if (state == NODESTATE_REACHABLE) {
			if (((n->n_addr[0] | n->n_addr[1]<<8) & 0xFC00) !=
			    (router_node & 0xFC00)) {
				rn = getnodebyaddr((char *)scratch_na, 2, AF_DECnet);
				if (!rn) {
				    rn = &scratch_n;
				    scratch_n.n_addr = scratch_na;
				    scratch_n.n_name = NULL;
				}
			}
			else {
				rn = n;
			}

			buf[ptr++] = 0x3e;  // 830=NEXT NODE
			buf[ptr++] = 0x03;

			buf[ptr++] = 0xc2; // What's this !?
			buf[ptr++] = 0x02; // Data type
			buf[ptr++] = rn->n_addr[0];
			buf[ptr++] = rn->n_addr[1];

			buf[ptr++] = 0x40;  // ASCII text
			if (rn && rn->n_name) {
				makeupper(rn->n_name);
				buf[ptr++] = strlen(rn->n_name);
				strcpy(&buf[ptr], rn->n_name);
				ptr += strlen(rn->n_name);
			}
			else {
				buf[ptr++] = 0;// No Name
			}

			// Also show the number of active links
			links = get_link_count(n->n_addr[0], n->n_addr[1]);
			if (links) {
				buf[ptr++] = 0x58; // 600=Active links
				buf[ptr++] = 0x2;  // Unsigned decimal

				buf[ptr++] = 2;    // Data length
				buf[ptr++] = links & 0xFFFF;
				buf[ptr++] = links >> 16;
			}
		}
	}

	/* Show exec details, name etc */
	if (exec) {
		struct utsname un;
		char ident[256];

		uname(&un);
		sprintf(ident, "%s V%s on %s", IDENT_STRING, un.release, un.machine);

		buf[ptr++] = 0x64;
		buf[ptr++] = 0;     // 100=Identification

		buf[ptr++] = 0x40;  // ASCII text
		buf[ptr++] = strlen(ident);
		strcpy(&buf[ptr], ident);
		ptr += strlen(ident);
	}
	return write(sock, buf, ptr);
}


static int send_exec(int sock)
{
	struct dn_naddr *exec_addr;
	struct nodeent *exec_node;
	unsigned int exec_area;
	unsigned int nodeaddr;
	char *dev;

	/* Get and send the exec information */
	exec_addr = getnodeadd();
	exec_area = exec_addr->a_addr[1]>>2;
	nodeaddr = exec_addr->a_addr[0] | exec_addr->a_addr[1]<<8;
	exec_node = getnodebyaddr((char*)exec_addr->a_addr, 2, AF_DECnet);
	dev = getexecdev();

	return send_node(sock, exec_node, 1, dev, NODESTATE_ON);
}

/* Save a neighbour entry in a list so we can check it when doing the
   KNOWN NODES display
 */
static int save_neigh(struct sockaddr_nl *who, struct nlmsghdr *n, void *arg)
{
        struct ndmsg *r = NLMSG_DATA(n);
        struct rtattr * tb[NDA_MAX+1];
        int sock = (int)arg;

        memset(tb, 0, sizeof(tb));
        parse_rtattr(tb, NDA_MAX, NDA_RTA(r), n->nlmsg_len - NLMSG_LENGTH(sizeof
(*r)));

        if (tb[NDA_DST])
	{
	    unsigned char *addr = RTA_DATA(tb[NDA_DST]);
	    if (++num_adj_nodes < MAX_ADJACENT_NODES)
	        adj_node[num_adj_nodes] = addr[0] | (addr[1]<<8);
    	}
	return 0;
}

/* Called for each neighbour node in the list - send it to the socket */
static int send_neigh(struct sockaddr_nl *who, struct nlmsghdr *n, void *arg)
{
	struct ndmsg *r = NLMSG_DATA(n);
	struct rtattr * tb[NDA_MAX+1];
	int sock = (int)arg;

	memset(tb, 0, sizeof(tb));
	parse_rtattr(tb, NDA_MAX, NDA_RTA(r), n->nlmsg_len - NLMSG_LENGTH(sizeof(*r)));

	if (tb[NDA_DST])
	{
		unsigned char *addr = RTA_DATA(tb[NDA_DST]);
		unsigned short faddr = addr[0] | (addr[1]<<8);
		struct nodeinfo *n, *n1;
		int interface = r->ndm_ifindex;
		int node = faddr & 0x3ff;
		int area = faddr >> 10;
		struct nodeent *ne = getnodebyaddr((char *)addr, 2, AF_DECnet);

		if (ne) {
			send_node(sock, ne, 0, if_index_to_name(interface), NODESTATE_REACHABLE);
		}
		else {
			struct nodeent tne;
			unsigned char tne_addr[2];

			tne.n_name = "";
			tne.n_addr = tne_addr;
			tne.n_addr[0] = addr[0];
			tne.n_addr[1] = addr[1];
			send_node(sock, &tne, 0, if_index_to_name(interface), NODESTATE_REACHABLE);
		}
	}
	return 0;
}

/* SHOW ADJACENT NODES */
static int get_neighbour_nodes(int sock, neigh_fn_t neigh_fn)
{
	router_node = get_router();

	if (first_time)
	{
		if (rtnl_open(&talk_rth, 0) < 0)
			return -1;
		if (rtnl_open(&listen_rth, 0) < 0)
			return -1;
		first_time = 0;
	}

	/* Get the list of adjacent nodes */
	if (rtnl_wilddump_request(&listen_rth, AF_DECnet, RTM_GETNEIGH) < 0) {
		syslog(LOG_ERR, "Cannot send dump request: %m");
		return -1;
	}

	/* Calls got_neigh() for each adjacent node */
	if (rtnl_dump_filter(&listen_rth, neigh_fn, (void *)sock, NULL, NULL) < 0) {
		syslog(LOG_ERR, "Dump terminated: %m\n");
		return -1;
	}
	dnetlog(LOG_DEBUG, "end of get_neighbour_nodes\n");
	return 0;
}

/* SHOW/LIST KNOWN NODES */
static int send_all_nodes(int sock, unsigned char perm_only)
{
	void *nodelist;
	char *nodename;

	send_exec(sock);

	/* Get adjacent nodes */
	num_adj_nodes = 0;
	if (!perm_only)
	    get_neighbour_nodes(sock, save_neigh);

	/* Now iterate the permanent database */
	nodelist = dnet_getnode();
	nodename = dnet_nextnode(nodelist);
	while (nodename)
	{
		struct nodeent *n = getnodebyname(nodename);

		send_node(sock, n, 0, NULL, adjacent_node(n)?NODESTATE_REACHABLE:NODESTATE_UNKNOWN);
		nodename = dnet_nextnode(nodelist);
	}
	dnet_endnode(nodelist);
	return 0;
}

/*
 * We read /proc/net/decnet and fill in the number of links to each node
 * that we find
 */
static int count_links(void)
{
	char buf[256];
	char var1[32];
	char var2[32];
	char var3[32];
	char var4[32];
	char var5[32];
	char var6[32];
	char var7[32];
	char var8[32];
	char var9[32];
	char var10[32];
	char var11[32];
	int i;
	FILE *procfile = fopen("/proc/net/decnet", "r");

	if (!procfile)
		return 0;

	while (!feof(procfile))
	{
		fgets(buf, sizeof(buf), procfile);
		if (sscanf(buf, "%s %s %s %s %s %s %s %s %s %s %s\n",
			   var1,var2,var3,var4,var5,var6,var7,var8, var9, var10, var11) == 11) {
			int area, node;
			struct link_node *lnode = NULL;

			/* In case we ever do "SHOW KNOWN LINKS:
			 * var10 is remote user, var5 is local user
			 */
			sscanf(var6, "%d.%d\n", &area, &node);

			/* Ignore 0.0 links (listeners) and anything not in RUN state */
			if (area == 0 || node == 0 || strcmp(var11, "RUN"))
				continue;

			for (i=0; i<num_link_nodes; i++) {
				if (link_nodes[i].area == area &&
				    link_nodes[i].node == node) {
					lnode = &link_nodes[i];
					break;
				}
			}
			if (!lnode && i < MAX_ADJACENT_NODES) {
				lnode = &link_nodes[num_link_nodes++];
				lnode->area = area;
				lnode->node = node;
			}
			if (lnode) {
				lnode->links++;
			}
		}
	}
	fclose(procfile);
	return 0;

}

static int read_information(int sock, unsigned char *buf, int length)
{
	unsigned char option = buf[1];
	unsigned char entity = buf[2];

	char response = 2;

	// Tell remote end we are sending the data.
	write(sock, &response, 1);

// Parameter entries from [3] onwards.

	// option & 0x80: 1=perm DB, 0=volatile DB
	// option & 0x78: type 0=summary, 1=status, 2=char, 3=counters, 4=events
	// option & 0x07: entity type

	// entity: 0=node, 1=line, 2=logging, 3=circuit, 4=module 5=area
	dnetlog(LOG_DEBUG, "option=%d. entity=%d\n", option, entity);

	switch (option & 0x7f)
	{
	case 0:  // nodes summary
	case 16: // nodes char
	case 32: // nodes state
		if (entity == 0xff) { // KNOWN NODES
			count_links();
			send_all_nodes(sock, option & 0x80);
		}
		if (entity == 0xfc) { // ADJACENT NODES
			count_links();
			get_neighbour_nodes(sock, send_neigh);
		}
		if (entity == 0x00)
			send_exec(sock);
		break;
	default:
		break;
	}

	// End of data.
	response = -128;
	write(sock, &response, 1);
	return 0;
}

static void unsupported(int sock)
{
	char buf[256];

	buf[0] = -1;
	buf[1] = 0;
	buf[2] = 0;
	// TODO This text should be of the defined type...
	strcpy(&buf[3], "Unrecognised command");

	write(sock, buf, strlen(&buf[3])+3);
}

int process_request(int sock, int verbosity)
{
	unsigned char buf[4096];
	int status;

	verbose = verbosity;
	do
	{
		status = read(sock, buf, sizeof(buf));
		if (status == -1 || status == 0)
			break;

		if (verbosity > 1)
		{
			int i;

			dnetlog(LOG_DEBUG, "Received message %d bytes: \n", status);
			for (i=0; i<status; i++)
				dnetlog(LOG_DEBUG, "%02x ", buf[i]);
			dnetlog(LOG_DEBUG, "\n");
		}

		switch (buf[0])
		{
		case 15://          Request down-line load
			unsupported(sock);
			break;
		case 16://          Request up-line dump
			unsupported(sock);
			break;
		case 17://          Trigger bootstrap
			unsupported(sock);
			break;
		case 18://          Test
			unsupported(sock);
			break;
		case 19://          Change parameter
			unsupported(sock);
			break;
		case 20://          Read information
			read_information(sock, buf, status);
			break;
		case 21://          Zero counters
			unsupported(sock);
			break;
		case 22://          System-specific function
			unsupported(sock);
			break;
		default:
			unsupported(sock);
			// Send error
		}

	} while (status > 0);

	return 0;
}
