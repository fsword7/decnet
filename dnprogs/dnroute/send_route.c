/*
 * dnroute.c    DECnet routing daemon (eventually...)
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
#include "dnrtlink.h"
#include "csum.h"

extern char *if_index_to_name(int ifindex);
extern int routing_multicast_timer;

/* A list of all possible nodes in our area. the byte is set to
   0: unavailable
   1: available
   2: us
*/
static unsigned char node_table[1024];


/* Send a packet to all ethernet interfaces */
int send_to_all(int dnet_socket, char *packet, int len, struct sockaddr_ll *sock_info)
{
    struct ifreq ifr;
    int iindex = 1;

    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    ifr.ifr_ifindex = iindex;

    while (ioctl(sock, SIOCGIFNAME, &ifr) == 0)
    {
	/* Only send to ethernet interfaces */
	ioctl(sock, SIOCGIFHWADDR, &ifr);		    
	if (ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER)
	{
	    sock_info->sll_ifindex = iindex;
	    if (sendto(dnet_socket, packet, len, 0,
		      (struct sockaddr *)sock_info, sizeof(*sock_info)) < 0)
		perror("sendto");
	}	    
	ifr.ifr_ifindex = ++iindex;
    }
    
    close(sock);
    return 0;
}


/* Send a routine message for nodes "start" to "end".
   "start" should be a multiple of 32 for the header to be
   added correctly */
static int send_routing_message(int start, int end, int dnet_socket, struct dn_naddr *exec)
{
    struct sockaddr_ll sock_info;
    unsigned char packet[1600];
    unsigned short sum;
    int i,j;

    i=0;

    packet[i++] = 0x9a;
    packet[i++] = 0x05;
    
    packet[i++] = 0x07; /* Level 1 routing message */
    packet[i++] = exec->a_addr[0]; /* Our node address */
    packet[i++] = exec->a_addr[1];
    packet[i++] = 0x00; /* Reserved */

    /* Do the nodes in blocks of 32 */
    for (j=start; j<end; j++)
    {
	if ( !(j & 0x3F))
	{
	    packet[i++] = 32;
	    packet[i++] = 0;
	    packet[i++] = j&0xFF;
	    packet[i++] = j>>8;
	}
	switch (node_table[j])
	{
	case 0: /* Unreachable */
	default:
	    packet[i++] = 0xff;
	    packet[i++] = 0x7f;
	    break;
	case 1: /* Reachable, cost 4, hops 2 */
	    packet[i++] = 0x04;
	    packet[i++] = 0x04;
	    break;
	case 2: /* Local, cost 0, hops 0 */
	    packet[i++] = 0x0;
	    packet[i++] = 0x0;
	    break;
	}
    }

    /* Add in checksum */
    sum = route_csum(packet, 4, i);
    packet[i++] = sum & 0xFF;
    packet[i++] = sum >> 8;

    /* Build the sockaddr_ll structure */
    sock_info.sll_family   = AF_PACKET;
    sock_info.sll_protocol = htons(ETH_P_DNA_RT);
    sock_info.sll_ifindex  = 2; /* TODO: DO ALL INTERFACES... */
    sock_info.sll_hatype   = 0;
    sock_info.sll_pkttype  = PACKET_MULTICAST;
    sock_info.sll_halen    = 6;

    /* This is the DECnet multicast address */
    sock_info.sll_addr[0]  = 0xab;
    sock_info.sll_addr[1]  = 0x00;
    sock_info.sll_addr[2]  = 0x00;
    sock_info.sll_addr[3]  = 0x03;
    sock_info.sll_addr[4]  = 0x00;
    sock_info.sll_addr[5]  = 0x00;

    /* Send to all interfaces */
    return send_to_all(dnet_socket, packet, i, &sock_info);
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

	int node = faddr & 0x3ff;

	if (!strcmp(if_index_to_name(r->ndm_ifindex), "lo"))
	    node_table[node] = 2;/* It's us! */
	else
	    node_table[node] = 1;/* Reachable */
    }

    return 0;
}

void send_route_msg(int dummy)
{
    int dnet_socket;
    struct rtnl_handle rth;
    struct dn_naddr *addr;

    /* Clear out the nodes table */
    memset(node_table, 0, sizeof(node_table));

    /* Get the list of adjacent nodes */
    if (rtnl_open(&rth, 0) < 0)
	goto resched;
    
    if (rtnl_wilddump_request(&rth, AF_DECnet, RTM_GETNEIGH) < 0) {
	perror("Cannot send dump request");
	goto resched;
    }
    
    if (rtnl_dump_filter(&rth, got_neigh, stdout, NULL, NULL) < 0) {
	fprintf(stderr, "Dump terminated\n");
	goto resched;
    }

    /* Build an L1 routing message from the node table */
    addr = getnodeadd();    
    dnet_socket = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_DNA_RT));

    /* 576 is a multiple of 32 so the mask in
       send_routine_message() works */
    send_routing_message(0, 575, dnet_socket, addr);
    send_routing_message(576, 1023, dnet_socket, addr);

    close(dnet_socket);
    
    /* Schedule us again */
 resched:
    alarm(routing_multicast_timer);
}
