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

/* Send a packet to all ethernet interfaces */
static int send_to_all(int dnet_socket, char *packet, int len, struct sockaddr_ll *sock_info)
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
static int send_routing_message(char *node_table, int start, int end,
				int dnet_socket, struct dn_naddr *exec)
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

void send_route_msg(char *node_table)
{
    int dnet_socket;
    struct rtnl_handle rth;
    struct dn_naddr *addr;

    /* Get our node address */
    addr = getnodeadd();
    dnet_socket = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_DNA_RT));

    /* 576 is a multiple of 32 so the mask in
       send_routine_message() works */
    send_routing_message(node_table, 0, 575, dnet_socket, addr);
    send_routing_message(node_table, 576, 1023, dnet_socket, addr);

    close(dnet_socket);
}