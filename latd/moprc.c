/******************************************************************************
    (c) 2001-2002 patrick Caulfield                 patrick@debian.org

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
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <regex.h>
#include <stdlib.h>
#include <utmp.h>
#include <grp.h>
#include <signal.h>
#include <assert.h>
#include <netinet/in.h>
#include <netinet/ether.h>
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

#include "moprc.h"

static int  mop_socket;
static char last_message[1500];
static int  last_message_len;
static int  show_info = 0;
static int  do_moprc(char *, int);
static int  send_boot(char *macaddr, int interface);

static int usage(FILE *f, char *cmd)
{
    fprintf(f, "\nUsage: %s [?hVv] [-i <interface>] <node name>|<macaddr>\n", cmd);

    fprintf(f, "   -?         Show this usage message\n");
    fprintf(f, "   -h         Show this usage message\n");
    fprintf(f, "   -V         Show the version of moprc\n");
    fprintf(f, "   -i         Ethernet interface to use (default eth0)\n");
    fprintf(f, "   -t         Trigger (reboot) the server\n");
    fprintf(f, "   -v         Show target information\n");
    fprintf(f, "\n");
    fprintf(f, "Node names are read from /etc/ethers\n");
    fprintf(f, "MAC addresses in colon-seperated form. eg:\n");
    fprintf(f, "\n%s -i eth1 08:00:2B:2B:AD:99\n", cmd);
    if (geteuid() != 0)
	fprintf(f, "\nYou will probably need to be root to run this program.\n");
    fprintf(f, "\n");
    return -1;
}

/* Find the interface named <ifname> and return it's number
   Return -1 if we didn't find it or it's not ethernet,
*/
static int find_interface(char *ifname)
{
    struct ifreq ifr;
    int iindex = 1;
    int sock = socket(PF_PACKET, SOCK_RAW, 0);

    ifr.ifr_ifindex = iindex;

    while (ioctl(sock, SIOCGIFNAME, &ifr) == 0)
    {
	if (strcmp(ifr.ifr_name, ifname) == 0)
	{
	    /* Also check it's ethernet while we are here */
	    ioctl(sock, SIOCGIFHWADDR, &ifr);
	    if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)
	    {
		fprintf(stderr, "Device %s is not ethernet\n", ifname);
		return -1;
	    }
	    close(sock);
	    return iindex;
	}
	ifr.ifr_ifindex = ++iindex;
    }
    /* Didn't find it */
    close(sock);
    return -1;
}


static int open_mop_socket(int interface)
{
    int mop_socket;
    struct sockaddr_ll sock_info;

    /* Open MOP protocol socket */
    mop_socket = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_DNA_RC));
    if (mop_socket < 0)
    {
	fprintf(stderr, "Can't create MOP protocol socket: %s\n", strerror(errno));
	exit(1);
    }


    /* Build the sockaddr_ll structure */
    sock_info.sll_family   = AF_PACKET;
    sock_info.sll_protocol = htons(ETH_P_DNA_RC);

    sock_info.sll_ifindex  = interface;
    if (bind(mop_socket, (struct sockaddr *)&sock_info, sizeof(sock_info)))
    {
	perror("can't bind MOPRC socket: %m\n");
	return -1;
    }

    return mop_socket;
}



/* Send a MOP message to a specified MAC address */
static int send_message(unsigned char *buf, int len, int interface, unsigned char *macaddr)
{
    struct sockaddr_ll sock_info;

    if (len < 46) len = 46;

    /* Build the sockaddr_ll structure */
    sock_info.sll_family   = AF_PACKET;
    sock_info.sll_protocol = htons(ETH_P_DNA_RC);
    sock_info.sll_ifindex  = interface;
    sock_info.sll_hatype   = 0;
    sock_info.sll_pkttype  = PACKET_MULTICAST;
    sock_info.sll_halen    = 6;
    memcpy(sock_info.sll_addr, macaddr, 6);

    memcpy(last_message, buf, len);
    last_message_len = len;

    if (sendto(mop_socket, buf, len, 0,
	       (struct sockaddr *)&sock_info, sizeof(sock_info)) < 0)
    {
	perror("Sending packet");
	return -1;
    }

    return 0;
}

static int send_last_message(int interface, unsigned char *macaddr)
{
    return send_message(last_message, last_message_len, interface, macaddr);
}

int main(int argc, char *argv[])
{
    int opt;
    int interface = find_interface("eth0");
    int trigger=0;
    struct ether_addr addr;

    if (argc < 2)
    {
        return usage(stdout, argv[0]);
    }

/* Get command-line options */
    opterr = 0;
    optind = 0;
    while ((opt=getopt(argc,argv,"?hVvti:")) != EOF)
    {
	switch(opt)
	{
	case 'h':
	    return usage(stdout, argv[0]);

	case '?':
	    return usage(stdout, argv[0]);

	case 'v':
	    show_info++;
	    break;

	case 't':
	    trigger++;
	    break;

	case 'i':
	    interface = find_interface(optarg);
	    if (interface == -1)
	    {
		fprintf(stderr, "Unknown interface: %s\n", optarg);
		return 2;
	    }
	    break;

	case 'V':
	    printf("\nMoprc version %s\n\n", VERSION);
	    exit(0);
	    break;
	}
    }

    if (!argv[optind])
    {
	return usage(stderr, argv[0]);
    }

    /* Check for a hostname in /etc/ethers */
    if (ether_hostton(argv[optind], &addr) != 0)
    {
	struct ether_addr *addr1;

	/* Otherwise parse ethernet MAC address */
	addr1 = ether_aton(argv[optind]);
	if (addr1)
	{
	    addr = *addr1;
	}
	else
	{
	    fprintf(stderr, "unknown node name or bad MAC address %s\n", argv[optind]);
	    return 3;
	}
    }

    mop_socket = open_mop_socket(interface);
    if (mop_socket == -1) exit(4);

    if (trigger)
    {
	return send_boot(addr.ether_addr_octet, interface);
    }

    return do_moprc(addr.ether_addr_octet, interface);
}

static int readmop(int mop_socket, char *buf, int buflen)
{
    struct msghdr msg;
    struct iovec iov;
    struct sockaddr_ll sock_info;
    int len;

    msg.msg_name = &sock_info;
    msg.msg_namelen = sizeof(sock_info);
    msg.msg_iovlen = 1;
    msg.msg_iov = &iov;
    iov.iov_len = buflen;
    iov.iov_base = buf;

    len = recvmsg(mop_socket, &msg, 0);
    if (len < 0)
    {
	if (errno != EINTR)
	    return -1;
	else
	    return 0;
    }

    /* Ignore packets captured in promiscuous mode. */
    if (sock_info.sll_pkttype == PACKET_OTHERHOST)
    {
	return 0;
    }
    return len;
}

static int send_reserve(char *macaddr, int interface)
{
    unsigned char buf[32];
    memset(buf, 0, sizeof(buf));

    buf[0] = 9;
    buf[1] = 0;
    buf[2] = MOPRC_CMD_RESERVE;

   return  send_message(buf, 11, interface, macaddr);
}

static int send_release(char *macaddr, int interface)
{
    unsigned char buf[32];
    memset(buf, 0, sizeof(buf));

    buf[0] = 1;
    buf[1] = 0;
    buf[2] = MOPRC_CMD_RELEASE;

   return  send_message(buf, 3, interface, macaddr);
}

static int send_reqid(char *macaddr, int interface)
{
    unsigned char buf[32];
    memset(buf, 0, sizeof(buf));

    buf[0] = 4;
    buf[1] = 0;
    buf[2] = MOPRC_CMD_REQUESTID;

   return  send_message(buf, 7, interface, macaddr);
}

static int send_boot(char *macaddr, int interface)
{
    unsigned char buf[32];
    memset(buf, 0, sizeof(buf));

    buf[0] = 12;
    buf[1] = 0;
    buf[2] = MOPRC_CMD_BOOT;
    buf[13] = 0xFF;

   return  send_message(buf, 14, interface, macaddr);
}

static int send_data(char *data, int len, char *macaddr, int interface)
{
    unsigned char buf[len+46];
    static char message = 0;

    memset(buf, 0, sizeof(buf));

    buf[0] = len+2;
    buf[1] = 0;
    buf[2] = MOPRC_CMD_COMMANDPOLL;
    buf[3] = message;
    memcpy(buf+4, data, len);

    message = 1-message;
    return send_message(buf, len+4, interface, macaddr);
}

static void print_ascic(char *buf, int len)
{
    int i;

    for (i=0; i <len; i++)
    {
	if (isprint(buf[i]))
	    printf("%c", buf[i]);
	else
	    printf(".");
    }
    printf("\n");
}

static int show_system_info(unsigned char *info, int len)
{
    int index=0;
    int functions = 0;

    while (index < len)
    {
	int type = info[index] | info[index+1]<<8;
	int infolen = info[index+2];

	index += 3;
	if (show_info || type == 2)
	{
	    switch (type)
	    {
	    case 1: /* Maintenance version */
		printf("Maintenance Version: %d.%d.%d\n",
		       info[index],info[index+1],info[index+2]);
		break;
	    case 2: /* Functions */
		functions = info[index];
		break;

		/* These are sent by terminal servers */
	    case 102:
		printf("ROM version:         ");
		print_ascic(&info[index], infolen);
		break;
	    case 103:
		printf("S/W version:         ");
		print_ascic(&info[index], infolen);
		break;
	    case 105:
		printf("Node Name:           ");
		print_ascic(&info[index], infolen);
		break;
	    case 106:
		printf("Identification:      ");
		print_ascic(&info[index], infolen);
		break;

		/* Other stuff */
	    case 201:
		printf("Operating System:    ");
		print_ascic(&info[index], infolen);
		break;
	    case 202:
		printf("Software Version:    ");
		print_ascic(&info[index], infolen);
		break;
	    case 203:
		printf("Node Name:           ");
		print_ascic(&info[index], infolen);
		break;

	    default:
		break;
	    }
	    index += infolen;
	}
    }
    if (show_info) printf("\n");
    return functions & 0x20; /* Do we do CCP? */
}

static int do_moprc(char *macaddr, int interface)
{
    enum {STARTING, CONNECTED} state=STARTING;
    fd_set         in;
    unsigned char  buf[1500];
    struct timeval tv;
    struct termios old_term;
    struct termios new_term;
    int            len;
    int            last_msg_tag = 99; /* Dummy */
    int            status;
    int            timeout = 200000; /* Poll interval */
    int            waiting_ack;
    int            resends = 0;
    int            termfd = STDIN_FILENO;

    tcgetattr(termfd, &old_term);
    new_term = old_term;

    /* Set local terminal characteristics */
    new_term.c_iflag &= ~BRKINT;
    new_term.c_iflag |= IGNBRK | INLCR;
    new_term.c_lflag &= ~ISIG;
    new_term.c_cc[VMIN] = 1;
    new_term.c_cc[VTIME] = 0;
    new_term.c_lflag &= ~ICANON;
    new_term.c_lflag &= ~(ECHO | ECHOCTL | ECHONL);
    tcsetattr(termfd, TCSANOW, &new_term);

    /* Send connect packets */
    send_reserve(macaddr, interface);
    send_reqid(macaddr, interface);
    waiting_ack = 1;

    /* Main loop */
    FD_ZERO(&in);
    FD_SET(mop_socket, &in);
    tv.tv_sec  = 5;
    tv.tv_usec = 0;

    /* Loop for input */
    while ( (status = select(mop_socket+1, &in, NULL, NULL, &tv)) >= 0)
    {
	/* No data, poll for any input from the terminal server */
	if (status == 0 && !waiting_ack)
	{
	    char dummybuf[1];
	    send_data(dummybuf, 0, macaddr, interface);
	    waiting_ack = 1;
	    resends = 0;
	}

	/* Waiting for an ACK but not got one. Resend the last
	   packet */
	if (status == 0 && waiting_ack)
	{
	    send_last_message(interface, macaddr);
	    if (++resends == 3)
	    {
		printf("\nTarget does not respond\n");
		break;
	    }
	}

	/* Data from the terminal server */
	if (FD_ISSET(mop_socket, &in))
	{
	    int cmd;
	    int datalen;

	    len = readmop(mop_socket, buf, sizeof(buf));
	    if (len < 0)
	    {
	        perror("reading from Remote");
		return -1;
	    }
	    if (len == 0) continue;

	    waiting_ack = 0;
	    cmd = buf[2];
	    datalen = buf[0] | buf[1] <<8;

	    /* Ignore duplicates */
	    if (last_msg_tag == buf[3])
		continue;
	    last_msg_tag = buf[3];

	    switch (cmd)
	    {
	    case MOPRC_CMD_RESERVE:
	    case MOPRC_CMD_RELEASE:
	    case MOPRC_CMD_REQUESTID:
		fprintf(stderr, "Got unsupported MOPRC function %d\n", cmd);
		break;

		/* Got some data to display */
	    case MOPRC_CMD_RESPONSE:
		if (datalen >= 2)
		{
		    int i;
		    for (i=0; i<datalen-2; i++)
		    {
		        fputc(buf[4+i], stdout);
		    }
		    fflush(stdout);
		}
		break;

		/* Response to our REQUESTID message means
		   we are connected */
	    case MOPRC_CMD_SYSTEMID:
		if (state == STARTING)
		{
		    if (show_system_info(&buf[6], datalen-1) == 0)
		    {
			printf("target does not support remote console\n");
			goto finished;
		    }

		    if (isatty(STDIN_FILENO))
			printf("Console connected (press CTRL/D when finished)\n");

		    state = CONNECTED;
		}
		break;
	    default:
		fprintf(stderr, "Got unknown MOPRC function %d\n", cmd);
		break;
	    }
	}

	/* Keyboard input */
	if (FD_ISSET(STDIN_FILENO, &in))
	{
	    int i;
	    len = read(STDIN_FILENO, buf, sizeof(buf));
	    if (len < 0)
	    {
	        perror("reading from stdin");
		return -1;
	    }
	    if (len == 0) break;

	    /* Convert LF to CR & check for ^D */
	    for (i=0; i<len; i++)
	    {
		if (buf[i] == 4) goto finished;
		if (buf[i] == '\n') buf[i] = '\r';
	    }
	    waiting_ack = 1;
	    resends = 0;
	    send_data(buf, len, macaddr, interface);
	}

	/* Reset for another select */
	tv.tv_usec  = timeout;
	tv.tv_sec = 0;

	FD_ZERO(&in);
	FD_SET(mop_socket, &in);
	if (!waiting_ack) FD_SET(STDIN_FILENO, &in);
    }

 finished:
    /* Send disconnect */
    send_release(macaddr, interface);

    tcsetattr(termfd, TCSANOW, &old_term);
    printf("\n");
    return 0;
}
