/******************************************************************************
    (c) 2001-2004 patrick Caulfield                 patrick@debian.org

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
#ifdef HAVE_NET_IF_ETHER_H
#include <net/if.h>
#include <net/if_ether.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#ifdef HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#endif

#include <string>

#include "interfaces.h"
#include "moprc.h"

#ifdef __FreeBSD__
#define ether_addr_octet octet
#endif

static int  mop_socket;
static unsigned char last_message[1500];
static int  last_message_len;
static int  show_info = 0;
static int  do_moprc(u_int8_t *, int, int, int);
static int  send_boot(u_int8_t *macaddr, int interface);
static LATinterfaces *iface;

static int usage(FILE *f, char *cmd)
{
    fprintf(f, "\nUsage: %s [?hVv] [-i <interface>] <node name>|<macaddr>\n", cmd);

    fprintf(f, "   -?         Show this usage message\n");
    fprintf(f, "   -h         Show this usage message\n");
    fprintf(f, "   -V         Show the version of moprc\n");
    fprintf(f, "   -i         Ethernet interface to use (default to first found)\n");
    fprintf(f, "   -t         Trigger (reboot) the server\n");
    fprintf(f, "   -b         Make ^H send DEL\n");
    fprintf(f, "   -v         Show target information\n");
    fprintf(f, "   -p <ms>    Set poll interval (default 200)\n");
    fprintf(f, "\n");
    fprintf(f, "Node names are read from /etc/ethers\n");
    fprintf(f, "MAC addresses in colon-seperated form. eg:\n");
    fprintf(f, "\n%s -i eth1 08:00:2B:2B:AD:99\n", cmd);
    if (geteuid() != 0)
	fprintf(f, "\nYou will probably need to be root to run this program.\n");
    fprintf(f, "\n");
    return -1;
}


/* Send a MOP message to a specified MAC address */
static int send_message(unsigned char *buf, int len, int interface, u_int8_t *macaddr)
{
    int status;
    if (len < 46) len = 46;

    status = iface->send_packet(interface, macaddr, buf, len);
    if (status < 0)
	    perror("send message");

    memmove(last_message, buf, len);
    last_message_len = len;
    return status;
}

static int send_last_message(int interface, u_int8_t *macaddr)
{
    return send_message(last_message, last_message_len, interface, macaddr);
}

int main(int argc, char *argv[])
{
    int opt;
    int interface = -1;
    int trigger=0;
    int convert_bs = 0;
    int poll_interval = 200; /* in ms */
    char ifname_buf[255];
    char *ifname;
    struct ether_addr addr;

    if (argc < 2)
    {
        return usage(stdout, argv[0]);
    }

    /* Look for MOPRC_INTERFACE environment variable */
    ifname = getenv("MOPRC_INTERFACE");

/* Get command-line options */
    opterr = 0;
    while ((opt=getopt(argc,argv,"?hVvtbi:p:")) != EOF)
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

	case 'b':
	    convert_bs++;
	    break;

	case 'i':
	    strcpy(ifname_buf, optarg);
	    ifname = ifname_buf;
	    break;

	case 'p':
	    poll_interval = atoi(optarg);
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
#ifndef __APPLE__
	struct ether_addr *addr1;

	/* Otherwise parse ethernet MAC address */
	addr1 = ether_aton(argv[optind]);
	if (addr1)
	{
	    addr = *addr1;
	}
	else
#endif
	{
	    fprintf(stderr, "unknown node name or bad MAC address %s\n", argv[optind]);
	    return 3;
	}
    }

    /* Initialise the platform-specific interface code */
    iface = LATinterfaces::Create();
    if (iface->Start(LATinterfaces::ProtoMOP) == -1)
    {
	fprintf(stderr, "Can't create MOP protocol socket: %s\n", strerror(errno));
	exit(1);
    }

    // If no interface on the command-line then use defaults
    interface = iface->find_interface(ifname);
    if (interface == -1)
    {
	if (ifname)
	    fprintf(stderr, "Cannot resolve interface %s\n", ifname);
	else
	    fprintf(stderr, "Cannot find any ethernet interfaces\n");
	return 2;
    }

    mop_socket = iface->get_fd(interface);
    if (iface->bind_socket(interface))
	return 3;

    if (trigger)
    {
	return send_boot(addr.ether_addr_octet, interface);
    }

    return do_moprc(addr.ether_addr_octet, interface, convert_bs, poll_interval*1000);
}

static int readmop(unsigned char *buf, int buflen)
{
    int ifn;
    unsigned char macaddr[6];
    bool more;

    return iface->recv_packet(mop_socket, ifn, macaddr, buf, buflen, more);
}

static int send_reserve(u_int8_t *macaddr, int interface)
{
    unsigned char buf[32];
    memset(buf, 0, sizeof(buf));

    buf[0] = 9;
    buf[1] = 0;
    buf[2] = MOPRC_CMD_RESERVE;

   return  send_message(buf, 11, interface, macaddr);
}

static int send_release(u_int8_t *macaddr, int interface)
{
    unsigned char buf[32];
    memset(buf, 0, sizeof(buf));

    buf[0] = 1;
    buf[1] = 0;
    buf[2] = MOPRC_CMD_RELEASE;

   return  send_message(buf, 3, interface, macaddr);
}

static int send_reqid(u_int8_t *macaddr, int interface)
{
    unsigned char buf[32];
    memset(buf, 0, sizeof(buf));

    buf[0] = 5;
    buf[1] = 0;
    buf[2] = MOPRC_CMD_REQUESTID;

   return  send_message(buf, 7, interface, macaddr);
}

static int send_boot(u_int8_t *macaddr, int interface)
{
    unsigned char buf[32];
    memset(buf, 0, sizeof(buf));

    buf[0] = 12;
    buf[1] = 0;
    buf[2] = MOPRC_CMD_BOOT;
    buf[13] = 0xFF;

   return  send_message(buf, 14, interface, macaddr);
}

static int send_data(unsigned char *data, int len, u_int8_t *macaddr, int interface)
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

static void print_ascic(unsigned char *buf, int len)
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

static int do_moprc(u_int8_t *macaddr, int interface, int convert_bs, int timeout)
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
    int            waiting_ack;
    int            resends = 0;
    int            termfd = STDIN_FILENO;
    int            stdin_is_tty;
    int            last_packet_was_empty=1;

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

    stdin_is_tty = isatty(STDIN_FILENO);

    /* Loop for input */
    while ( (status = select(FD_SETSIZE, &in, NULL, NULL, &tv)) >= 0)
    {
	/* No response after 3 tries */
	if (waiting_ack && resends >= 3)
	{
	    printf("\nTarget does not respond\n");
	    break;
	}

	/* Waiting for an ACK but not got one. Resend the last
	   packet */
	if (status == 0 && waiting_ack)
	{
	    send_last_message(interface, macaddr);
	    resends++;
	    continue;
	}

	/* No data, poll for any input from the terminal server */
	if (status == 0 && !waiting_ack)
	{
	    unsigned char dummybuf[1];
	    send_data(dummybuf, 0, macaddr, interface);
	    waiting_ack = 1;
	    resends = 0;
	}

	/* Data from the terminal server */
	if (FD_ISSET(mop_socket, &in))
	{
	    int cmd;
	    int datalen;

	    len = readmop(buf, sizeof(buf));
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
		if (datalen > 2)
		{
		    fwrite(buf+4, datalen-2, 1, stdout);
		    fflush(stdout);
		    last_packet_was_empty = 0;
		}
		else
		{
		    last_packet_was_empty = 1;
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

		    if (stdin_is_tty)
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
	    len = read(STDIN_FILENO, buf, stdin_is_tty ? sizeof(buf) : 1);
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
		if (convert_bs && buf[i] == '\b') buf[i] = 127;
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
	if (!waiting_ack && last_packet_was_empty) FD_SET(STDIN_FILENO, &in);
    }

 finished:
    /* Send disconnect */
    send_release(macaddr, interface);

    tcsetattr(termfd, TCSANOW, &old_term);
    printf("\n");
    return 0;
}
