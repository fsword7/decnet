/******************************************************************************
    (c) 2006      P.J. Caulfield          patrick@debian.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    *******************************************************************************


    Multinet server proxy server for Linux.

    This daemon talks to a VMS system running a multinet DECnet/IP tunnel.
    (www.process.com/tcpip/multinet.html)

    You'll need to set up the VMS end to talk to this box.
    It creates a tun/tap device and assigns it a suitable
    MAC address.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/signal.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>

static int verbose;
static int ipfd;
static int tunfd;
static unsigned short local_addr[2];
static struct sockaddr_in remote_addr;
static int got_remote_addr;
static unsigned char remote_decnet_addr[2];
static int got_verification;

#define PORT 700

#define DUMP_MAX 1024

static int send_ip(int use_seq, unsigned char *, int len);

static void dump_data(char *from, unsigned char *databuf, int datalen)
{
	int i;
	if (!verbose)
		return;

	fprintf(stderr, "%s (%d)", from, datalen);
	for (i=0; i<(datalen>DUMP_MAX?DUMP_MAX:datalen); i++)
	{
		fprintf(stderr, "%02x  ", databuf[i]);
	}
	fprintf(stderr, "\n");
}

static void send_start(unsigned short addr)
{
	unsigned char start[] = { 0x01, 0x08, 0x05, 0x05, 0x40,0x02, 0x02,0x00, 0x00,0x2c, 0x01,0x00, 0x00,0x00,};
	unsigned char verf[] =  { 0x03, 0x02, 0x0c, 0x00};

	start[1] = addr & 0xff;
	start[2] = addr >> 8;

	send_ip(0, start, sizeof(start));

	verf[1] = addr & 0xff;
	verf[2] = addr >> 8;

	send_ip(0, verf, sizeof(verf));

}


static int send_tun(int mcast, unsigned char *buf, int len)
{
	unsigned char header[38];
	struct iovec iov[2];

	if (!got_remote_addr)
		return 0; /* Can't send yet */

	memset(header, 0, sizeof(header));

	// Add ethernet header
	header[0] = 0xAA;
	header[1] = 0x00;
	header[2] = 0x04;
	header[3] = 0x00;
	header[4] = local_addr[0];
	header[5] = local_addr[1];

	if (mcast) /* Routing muticast - type may need to be in callers params */
	{
		header[6] = 0x09;
		header[7] = 0x00;
		header[8] = 0x00;
		header[9] = 0x00;
		header[10] = 0x09;
		header[11] = 0x23;
	}
	else
	{
		header[6] = 0xAA;
		header[7] = 0x00;
		header[8] = 0x04;
		header[9] = 0x00;
		header[10] = remote_decnet_addr[0];
		header[11] = remote_decnet_addr[1];
	}
	header[12] = 0x60; /* DECnet packet type */
	header[13] = 0x03;

	if (!mcast)
	{
		/* DECnet packet length */
		header[14] = (len+16) & 0xFF;
		header[15] = (len+16) >> 8;

		/* Fake DECnet header */
		header[18] = header[19] = 0;
		header[16] = 0x81; header[17] = 0x26;  // Don't know what this is!

		header[20] = header[28] = 0xAA;
		header[21] = header[29] = 0x00;
		header[22] = header[30] = 0x04;
		header[23] = header[31] = 0x00;
		header[24] = buf[1]; // Dest addr
		header[25] = buf[2];
		header[32] = buf[3]; // src addr
		header[33] = buf[4];

		buf += 6;
		len -= 6;
	}

	iov[0].iov_base = header;
	iov[0].iov_len = sizeof(header);
	iov[1].iov_base = buf;
	iov[1].iov_len = len;

	dump_data("to TUN0:", header, sizeof(header));
	dump_data("to TUN1:", buf+6, len-6);

	writev(tunfd, iov, 2);
	return len;
}

static int send_ip(int fudge_header, unsigned char *buf, int len)
{
	struct iovec iov[2];
	unsigned char header[4];
	struct msghdr msg;
	static unsigned short seq;

	memset(&msg, 0, sizeof(msg));
	seq++;
	header[0] = seq & 0xFF;
	header[1] = seq >> 8;
	header[2] = header[3] = 0;

	if (fudge_header)
	{
		/* Shorten DECnet addresses */
		buf[0] = 0x02; // TODO what is this ??
		buf[1] = buf[8]; // Destination
		buf[2] = buf[9];
		buf[3] = buf[16]; // Source
		buf[4] = buf[17];
		buf[5] = 0;
		memmove(buf+6, buf+22, len-16);

		len -= 16;
	}

	iov[0].iov_base = header;
	iov[0].iov_len = sizeof(header);
	iov[1].iov_base = buf;
	iov[1].iov_len = len;

	msg.msg_iov = iov;
	msg.msg_iovlen = 2;
	msg.msg_name = (void *)&remote_addr;
	msg.msg_namelen = sizeof(struct sockaddr_in);

//	dump_data("Send to IP0", header, sizeof(header));
	dump_data("Send to IP1:", buf, len);

	if (sendmsg(ipfd, &msg, 0) <= 0)
		perror("sendmsg");

	return 0;
}


static int setup_ip(int port)
{
	int fd;
	struct sockaddr_in sin;

	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0)
		return -1;

	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = INADDR_ANY;

	if (bind(fd, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)))
	{
		perror("bind");
		close(fd);
		return -1;
	}

	return fd;
}

void resend_start(int sig)
{
	if (!got_verification)
	{
		unsigned short addr = (local_addr[0] | local_addr[1]<<8);
		send_start(addr);
		alarm(10);
	}
}

static void read_ip(void)
{
	unsigned char buf[1600];
	int len;

	len = read(ipfd, buf, sizeof(buf));
	if (len <= 0) return;

	dump_data("from IP:", buf, len);


	/* TODO Will need to resend this if other packets timeout */
	if (buf[4] == 0x05)
	{
		got_verification = 1;
		alarm(0);

		// TODO for '0x05' send hello packet
	}

	/* Trap INIT & VERF messages */
	if (buf[4] == 0x01 || buf[4] == 0x05)
	{
		if (!got_remote_addr)
		{
			unsigned short addr = buf[6]<<8 | buf[5];
			got_remote_addr = 1;
			remote_decnet_addr[0] = buf[5];
			remote_decnet_addr[1] = buf[6];

			printf("Remote address = %d.%d (%d)\n", addr>>10, addr&1023, addr);
		}

		return;
	}
	if (buf[4] == 0x03)
		return;
	if (buf[4] == 0x07) /* Routing info */
	{
		/*
		  off ethernet:
		  13:17:58.021480 lev-2-routing src 3.35 {areas 1-64 cost 4 hops 1}
		  0x0000:  8800 0923 0c00 3f00 0100 0404 0a04 0000
		  0x0010:  ff7f ff7f ff7f ff7f ff7f ff7f ff7f 0404
		  0x0020:  ff7f ff7f ff7f ff7f ff7f ff7f ff7f ff7f
		  0x0030:  ff7f ff7f ff7f ff7f ff7f ff7f 2428 1e08
		  0x0040:  ff7f ff7f ff7f ff7f ff7f ff7f ff7f ff7f
		  0x0050:  ff7f ff7f ff7f ff7f ff7f ff7f ff7f ff7f
		  0x0060:  ff7f ff7f ff7f ff7f ff7f ff7f ff7f ff7f
		  0x0070:  ff7f ff7f ff7f ff7f ff7f ff7f ff7f 1408
		  0x0080:  ff7f ff7f ff7f ff7f 8d44

		  off Multinet:
		  09  00  00  00  09  23
		  0c  00  3f  00  01  00  04  04  0a  04  00  00
		  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  04  04
		  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f
		  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  04  04  1e  08
		  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f
		  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f
		  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f
		  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  ff  7f  14  08
		  ff  7f  ff  7f  ff  7f  ff  7f  6d  20
		*/
		buf[2] = len % 0xFF;
		buf[3] = len >> 8;
		send_tun(1, buf+2, len-2);
		return;
	}

	send_tun(0, buf+4, len-4);
}

static void read_tun(void)
{
	unsigned char buf[1600];
	int len;

	len = read(tunfd, buf, sizeof(buf));
	if (len <= 0) return;

	/* Only send DECnet packets... */
	if (buf[12] == 0x60 && buf[13] == 0x03)
	{
		dump_data("DECnet from TUN:", buf, len);

		/* Ignore our echoed packets */
		if (buf[4] == local_addr[0] &&
		    buf[5] == local_addr[1])
		{
			if (verbose)
				fprintf(stderr, "Ignoring our own packet\n");
			return;
		}

		if (buf[16] == 0x0d) // Ethernet Hello, (TODO check routing ones too ?)
		{
			unsigned char ptp_hello[] = { 0x5, buf[10], buf[11], 0252, 0252, 0252, 0252};
			if (verbose)
				fprintf(stderr, "Sending PTP hello\n");
			send_ip(0, ptp_hello, sizeof(ptp_hello));
			return ;
		}

		send_ip(1, buf+16, len-16);
	}
}

static int setup_tun(unsigned short addr)
{
	int ret;
	struct ifreq ifr;
	char cmd[132];

	tunfd = open("/dev/net/tun", O_RDWR);
	if (tunfd < 0)
	{
		perror("could not open /dev/net/tun");
		exit(2);
	}
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	strcpy(ifr.ifr_name, "tap%d");
	ret = ioctl(tunfd, TUNSETIFF, (void *) &ifr);
	if (ret != 0)
	{
		perror("could not configure /dev/net/tun");
		exit(2);
	}
	fprintf(stderr, "using tun device %s\n", ifr.ifr_name);

	sprintf(cmd, "/sbin/ifconfig %s hw ether AA:00:04:00:%02X:%02X allmulti mtu 576 up\n", ifr.ifr_name, addr & 0xFF, addr>>8);
	system(cmd);

	return tunfd;
}

int main(int argc, char *argv[])
{
	struct pollfd pfds[2];
	unsigned int area, node;
	unsigned short addr;
	struct addrinfo *ainfo;
	struct addrinfo ahints;

	if (argc > 2 && strcmp(argv[1], "-v") == 0)
	{
		argv++;
		argc--;
		verbose = 1;
	}

	if (argc < 3)
	{
		printf("Usage: %s [-v] <remote-addr> <decnet-addr>\n", argv[0]);
		printf("eg     %s zarqon 3.2\n", argv[0]);
		exit(1);
	}

	if (sscanf(argv[2], "%d.%d", &area, &node) != 2)
	{
		fprintf(stderr, "DECnet address not valid\n");
		return -1;
	}
	if (area > 63 || node > 1023)
	{
		fprintf(stderr, "DECnet address not valid\n");
		return -1;
	}

	memset(&ahints, 0, sizeof(ahints));
	ahints.ai_socktype = SOCK_DGRAM;
	ahints.ai_protocol = IPPROTO_UDP;

	/* Lookup the nodename address */
	if (getaddrinfo(argv[1], NULL, &ahints, &ainfo))
	{
		perror("getaddrinfo");
		return -errno;
	}

	memcpy(&remote_addr, ainfo->ai_addr, sizeof(struct sockaddr_in));
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(PORT);


	addr = (area<<10) | node;
	local_addr[0] = addr & 0xFF;
	local_addr[1] = addr >> 8;

	tunfd = setup_tun(addr);
	ipfd = setup_ip(PORT);

	signal(SIGALRM, resend_start);
	alarm(10);
	send_start(addr);

	pfds[0].fd = ipfd;
	pfds[0].events = POLLIN;

	pfds[1].fd = tunfd;
	pfds[1].events = POLLIN;

	fcntl(ipfd, F_SETFL, fcntl(ipfd, F_GETFL, 0) | O_NONBLOCK);
	fcntl(tunfd, F_SETFL, fcntl(tunfd, F_GETFL, 0) | O_NONBLOCK);

	while (1)
	{
		int status;

		status = poll(pfds, 2, -1);
		if (status == -1 && errno != EINTR)
		{
			perror("poll");
			exit(1);
		}
		if (pfds[0].revents & POLLIN)
			read_ip();
		if (pfds[1].revents & POLLIN)
			read_tun();
	}

	return 0;
}
