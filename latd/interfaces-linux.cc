/******************************************************************************
    (c) 2002-2004 Patrick Caulfield                 patrick@debian.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
******************************************************************************/
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <syslog.h>
#include <ctype.h>
#include <regex.h>
#include <stdlib.h>
#include <utmp.h>
#include <grp.h>
#include <signal.h>
#include <assert.h>
#include <netinet/in.h>
#include <features.h>    /* for the glibc version number */
#if (__GLIBC__ >= 2 && __GLIBC_MINOR__ >= 1) || __GLIBC__ >= 3
#include <netpacket/packet.h>
#include <net/ethernet.h>     /* the L2 protocols */
#include <net/if_arp.h>
#include <linux/if.h>
#else
#include <asm/types.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>   /* The L2 protocols */
#endif

#include <list>
#include <queue>
#include <map>
#include <string>
#include <algo.h>
#include <iterator>
#include <strstream>
#include <iomanip>

#include "utils.h"
#include "interfaces.h"
#include "interfaces-linux.h"

int LATinterfaces::ProtoLAT = ETH_P_LAT;
int LATinterfaces::ProtoMOP = ETH_P_DNA_RC;

int LinuxInterfaces::Start(int proto)
{
    protocol = proto;

    // Open raw socket on specified protocol
    fd = socket(PF_PACKET, SOCK_DGRAM, htons(protocol));
    if (fd < 0)
    {
	syslog(LOG_ERR, "Can't create protocol socket: %m\n");
	return -1;
    }
    return 0;
}

// Return a list of valid interface numbers and the count
void LinuxInterfaces::get_all_interfaces(int *ifs, int &num)
{
    struct ifreq ifr;
    int iindex = 1;
    int sock = socket(PF_PACKET, SOCK_RAW, 0);
    num = 0;

    for (iindex = 1; iindex < 256; iindex++)
    {
	ifr.ifr_ifindex = iindex;
        if (ioctl(sock, SIOCGIFNAME, &ifr) == 0)
	{
	    // Only use ethernet interfaces
	    ioctl(sock, SIOCGIFHWADDR, &ifr);
	    if (ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER)
	    {
		debuglog(("interface %d: %d\n", num, iindex));
		ifs[num++] = iindex;
	    }
	}
    }

    close(sock);

}

// Print the name of an interface
std::string LinuxInterfaces::ifname(int ifn)
{
    struct ifreq ifr;
    int sock = socket(PF_PACKET, SOCK_RAW, 0);

    ifr.ifr_ifindex = ifn;

    if (ioctl(sock, SIOCGIFNAME, &ifr) == 0)
    {
	close(sock);
	return std::string((char *)ifr.ifr_name);
    }

    // Didn't find it
    close(sock);
    return std::string("");
}

// Bind a socket to an interface
int LinuxInterfaces::bind_socket(int interface)
{
    struct sockaddr_ll sock_info;

    sock_info.sll_family   = AF_PACKET;
    sock_info.sll_protocol = htons(protocol);

    sock_info.sll_ifindex  = interface;
    if (bind(fd, (struct sockaddr *)&sock_info, sizeof(sock_info)))
    {
        perror("can't bind socket to i/f %m\n");
        return -1;
    }
    return 0;
}

// Find an interface number by name, or
// use the first one if name is NULL.
int LinuxInterfaces::find_interface(char *name)
{
    struct ifreq ifr;
    int iindex = 1;
    int sock = socket(PF_PACKET, SOCK_RAW, 0);

    // Default "1st" interface
    if (!name) name = "eth0";

    ifr.ifr_ifindex = iindex;

    for (iindex = 1; iindex < 256; iindex++)
    {
	ifr.ifr_ifindex = iindex;
	if (ioctl(sock, SIOCGIFNAME, &ifr) == 0)
	{
	    if (strcmp(ifr.ifr_name, name) == 0)
	    {
		// Also check it's ethernet while we are here
		ioctl(sock, SIOCGIFHWADDR, &ifr);
		if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)
		{
		    syslog(LOG_ERR, "Device %s is not ethernet\n", name);
		    return -1;
		}
		close(sock);
		return iindex;
	    }
	}
    }
    // Didn't find it
    close(sock);
    return -1;
}

// true if this class defines one FD for each active
// interface, false if one fd is used for all interfaces.
bool LinuxInterfaces::one_fd_per_interface()
{
    return false;
}

// Return the FD for this interface (will only be called once for
// select if above returns false)
int LinuxInterfaces::get_fd(int ifn)
{
    return fd;
}

// Send a packet to a given macaddr
int LinuxInterfaces::send_packet(int ifn, unsigned char macaddr[], unsigned char *data, int len)
{
    struct sockaddr_ll sock_info;

    /* Build the sockaddr_ll structure */
    sock_info.sll_family   = AF_PACKET;
    sock_info.sll_protocol = htons(protocol);
    sock_info.sll_ifindex  = ifn;
    sock_info.sll_hatype   = 0;//ARPHRD_ETHER;
    sock_info.sll_pkttype  = PACKET_MULTICAST;
    sock_info.sll_halen    = 6;
    memcpy(sock_info.sll_addr, macaddr, 6);

    return sendto(fd, data, len, 0,
		  (struct sockaddr *)&sock_info, sizeof(sock_info));
}

// Receive a packet from a given interface
int LinuxInterfaces::recv_packet(int sockfd, int &ifn, unsigned char macaddr[], unsigned char *data, int maxlen, bool &more)
{
    struct msghdr msg;
    struct iovec iov;
    struct sockaddr_ll sock_info;
    int    len;

/* Linux only returns 1 packet at a time */
    more = false;

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &sock_info;
    msg.msg_namelen = sizeof(sock_info);
    msg.msg_iovlen = 1;
    msg.msg_iov = &iov;
    iov.iov_len = maxlen;
    iov.iov_base = data;

    len = recvmsg(sockfd, &msg, 0);

    ifn = sock_info.sll_ifindex;
    memcpy(macaddr, sock_info.sll_addr, 6);

    // Ignore packets captured in promiscuous mode.
    if (sock_info.sll_pkttype == PACKET_OTHERHOST)
    {
	debuglog(("Got a rogue packet .. interface probably in promiscuous mode\n"));
	return 0;
    }
    return len;
}

// Open a connection on an interface
// Only necessary for LAT sockets.
int LinuxInterfaces::set_lat_multicast(int ifn)
{
    // Add Multicast membership for LAT on socket
    struct packet_mreq pack_info;

    /* Fill in socket options */
    pack_info.mr_type        = PACKET_MR_MULTICAST;
    pack_info.mr_alen        = 6;
    pack_info.mr_ifindex     = ifn;

    /* This is the LAT multicast address */
    pack_info.mr_address[0]  = 0x09;
    pack_info.mr_address[1]  = 0x00;
    pack_info.mr_address[2]  = 0x2b;
    pack_info.mr_address[3]  = 0x00;
    pack_info.mr_address[4]  = 0x00;
    pack_info.mr_address[5]  = 0x0f;


    if (setsockopt(fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
		   &pack_info, sizeof(pack_info)))
    {
	syslog(LOG_ERR, "can't add lat socket multicast : %m\n");
	return -1;
    }

    /* This is the LAT solicit address */
    pack_info.mr_address[0]  = 0x09;
    pack_info.mr_address[1]  = 0x00;
    pack_info.mr_address[2]  = 0x2b;
    pack_info.mr_address[3]  = 0x02;
    pack_info.mr_address[4]  = 0x01;
    pack_info.mr_address[5]  = 0x04;

    if (setsockopt(fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
		   &pack_info, sizeof(pack_info)))
    {
	syslog(LOG_ERR, "can't add lat socket multicast : %m\n");
	return -1;
    }

    return 0;
}

// Close an interface.
int LinuxInterfaces::remove_lat_multicast(int ifn)
{
    // Add Multicast membership for LAT on socket
    struct packet_mreq pack_info;

    /* Fill in socket options */
    pack_info.mr_type        = PACKET_MR_MULTICAST;
    pack_info.mr_alen        = 6;
    pack_info.mr_ifindex     = ifn;

    /* This is the LAT multicast address */
    pack_info.mr_address[0]  = 0x09;
    pack_info.mr_address[1]  = 0x00;
    pack_info.mr_address[2]  = 0x2b;
    pack_info.mr_address[3]  = 0x00;
    pack_info.mr_address[4]  = 0x00;
    pack_info.mr_address[5]  = 0x0f;

    if (setsockopt(fd, SOL_PACKET, PACKET_DROP_MEMBERSHIP,
		   &pack_info, sizeof(pack_info)))
    {
	syslog(LOG_ERR, "can't remove socket multicast : %m\n");
	return -1;
    }

    /* This is the LAT solicit address */
    pack_info.mr_address[0]  = 0x09;
    pack_info.mr_address[1]  = 0x00;
    pack_info.mr_address[2]  = 0x2b;
    pack_info.mr_address[3]  = 0x02;
    pack_info.mr_address[4]  = 0x01;
    pack_info.mr_address[5]  = 0x04;

    if (setsockopt(fd, SOL_PACKET, PACKET_DROP_MEMBERSHIP,
		   &pack_info, sizeof(pack_info)))
    {
	syslog(LOG_ERR, "can't remove socket multicast : %m\n");
	return -1;
    }

    return 0;
}

// Here's where we know how to instantiate the class.
LATinterfaces *LATinterfaces::Create()
{
    return new LinuxInterfaces();
}
