/*
 * get_neigh.c  DECnet routing daemon (eventually...)
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:     Patrick Caulfield <patrick@ChyGwyn.com>
 *              based on rtmon.c by Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <search.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <string.h>
#include <linux/netfilter_decnet.h>
#include <netdnet/dnetdb.h>
#include <features.h>    /* for the glibc version number */
#if (__GLIBC__ >= 2 && __GLIBC_MINOR__ >= 1) || __GLIBC__ >= 3
#include <netpacket/packet.h>
#include <net/ethernet.h>     /* the L2 protocols */
#include <net/if.h>
#include <net/if_arp.h>
#else
#include <asm/types.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>   /* The L2 protocols */
#endif

#include "utils.h"
#include "libnetlink.h"
#include "csum.h"


/* A list of all possible nodes in our area. the byte is set to
   0: unavailable
   1: available
   2: us
*/
static unsigned char node_table[1024];
extern char *if_index_to_name(int ifindex);
extern int routing_multicast_timer;
extern int send_route_msg(char *);

int verbose;
static int send_routing;

static struct rtnl_handle talk_rth;
static struct rtnl_handle listen_rth;
static int first_time = 1;


/* Add or replace route to a node */
static int add_route(unsigned short node, int interface)
{
    struct {
	struct nlmsghdr         n;
	struct rtmsg            r;
	char                    buf[1024];
    } req;

    memset(&req, 0, sizeof(req));

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST|NLM_F_CREATE|NLM_F_REPLACE;
    req.n.nlmsg_type = RTM_NEWROUTE;
    req.r.rtm_family = AF_DECnet;
    req.r.rtm_table = RT_TABLE_MAIN;
    req.r.rtm_protocol = RTPROT_BOOT;
    req.r.rtm_scope = RT_SCOPE_LINK;
    req.r.rtm_type = RTN_UNICAST;
    req.r.rtm_dst_len = 16;

    ll_init_map(&talk_rth);

    addattr_l(&req.n, sizeof(req), RTA_DST, &node, 2);
    addattr32(&req.n, sizeof(req), RTA_OIF, interface);

    return rtnl_talk(&talk_rth, &req.n, 0, 0, NULL, NULL, NULL);
}


/* Called for each neighbour node in the list */
static int got_neigh(struct sockaddr_nl *who, struct nlmsghdr *n, void *arg)
{
    struct ndmsg *r = NLMSG_DATA(n);
    struct rtattr * tb[NDA_MAX+1];

    memset(tb, 0, sizeof(tb));
    parse_rtattr(tb, NDA_MAX, NDA_RTA(r), n->nlmsg_len - NLMSG_LENGTH(sizeof(*r)));

    if (tb[NDA_DST])
    {
	unsigned char *addr = RTA_DATA(tb[NDA_DST]);
	unsigned short faddr = addr[0] | (addr[1]<<8);
	char nodekey[3];
	int interface = r->ndm_ifindex;
	ENTRY *e,s;

	int node = faddr & 0x3ff;


	if (verbose)
	    printf("got node %d.%d on %s\n", faddr>>10, node, if_index_to_name(interface));

	/* Look it up in the hash table */
	nodekey[0] = addr[0];
	nodekey[1] = addr[1];
	nodekey[2] = '\0';
	s.key = nodekey;
	e = hsearch(s, FIND);

	/* If it's not there or the interface has changed then
	   update the routing table */
	if (!e || (int)e->data != interface)
	{
	    ENTRY e1;
	    if (!e)
	    {
		e = &e1;
		e->key = nodekey;
		e->data = (void *)interface;
	    }

	    /* Update hash table */
	    hsearch(*e, ENTER);

	    /* update the node table */
	    if (interface == 0)
		node_table[node] = 2; /* Us */
	    else
		node_table[node] = 1;

	    /* TODO remove old nodes */

	    /* Set route using netlink */
	    if (add_route(faddr, interface) < 0)
		printf("Add route failed\n");
	}
    }

    return 0;
}

static void get_neighbours(int dummy)
{
    if (first_time)
    {
	if (rtnl_open(&talk_rth, 0) < 0)
	    goto resched;
	if (rtnl_open(&listen_rth, 0) < 0)
	    goto resched;
	first_time = 0;
    }

    /* Get the list of adjacent nodes */
    if (rtnl_wilddump_request(&listen_rth, AF_DECnet, RTM_GETNEIGH) < 0) {
	perror("Cannot send dump request");
	goto resched;
    }

    if (rtnl_dump_filter(&listen_rth, got_neigh, stdout, NULL, NULL) < 0) {
	fprintf(stderr, "Dump terminated\n");
	goto resched;
    }

    if (send_routing)
	send_route_msg(node_table);

    /* Schedule us again */
 resched:
    alarm(routing_multicast_timer);
}

static void usage(char *cmd, FILE *f)
{

    fprintf(f, "\nusage: %s [OPTIONS]\n\n", cmd);
    fprintf(f, " -h           Print this help message\n");
    fprintf(f, " -d           Don't fork into background\n");
    fprintf(f, " -r           Send DECnet routing messages\n");
    fprintf(f, " -V           Show program version\n");
    fprintf(f, "\n");
}

int main(int argc, char **argv)
{
    int opt;
    int no_daemon=0;

    while ((opt=getopt(argc,argv,"?Vvhrd")) != EOF)
    {
	switch(opt) {
	case 'h':
	    usage(argv[0], stdout);
	    exit(0);

	case '?': // Called if getopt doesn't recognise the option
	    usage(argv[0], stderr);
	    exit(0);

	case 'v':
	    verbose++;
	    break;

	case 'd':
	    no_daemon++;
	    break;

	case 'r':
	    send_routing++;
	    break;

	case 'V':
	    printf("\ndnroute from dnprogs version %s\n\n", VERSION);
	    exit(1);
	    break;
	}
    }

    if (!no_daemon)
    {
	pid_t pid;
	switch ( pid=fork() )
	{
	case -1:
	    perror("dnroute: can't fork");
	    exit(2);

	case 0: // child
	    break;

	default: // Parent.
	    if (verbose > 1) printf("server: forked process %d\n", pid);
	    exit(0);
	}

	// Detach ourself from the calling environment
	int devnull = open("/dev/null", O_RDWR);
	close(0);
	close(1);
	close(2);
	setsid();
	dup2(devnull, 0);
	dup2(devnull, 1);
	dup2(devnull, 2);
	chdir("/");
    }

    /* initialise the hash table */
    hcreate(1024);

    signal(SIGALRM, get_neighbours);

    /* Start it off */
    get_neighbours(0);

    while(1)
	pause();

    exit(0);
}
