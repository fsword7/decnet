/******************************************************************************
    (c) 2001-2002 Patrick Caulfield                 patrick@debian.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
******************************************************************************/

// LAT login Program (llogin)

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
#include <syslog.h>
#include <ctype.h>
#include <regex.h>
#include <stdlib.h>
#include <utmp.h>
#include <signal.h>
#include <limits.h>
#include <assert.h>
#include <termios.h>

#include <list>
#include <queue>
#include <map>
#include <string>
#include <algo.h>
#include <iterator>
#include <string>
#include <strstream>

#include "lat.h"
#include "latcp.h"
#include "utils.h"
#include "dn_endian.h"

static int latcp_socket;

static void make_upper(char *str);
static int  read_reply(int fd, int &cmd, unsigned char *&cmdbuf, int &len);
static bool send_msg(int fd, int cmd, char *buf, int len);
static bool open_socket(bool);
static int  terminal(int latfd, int, int, int, int);
static int  do_use_port(char *service, int quit_char, int crlf, int bsdel, int lfvt);
static int usage(char *cmd)
{
    printf ("Usage: llogin [<option>] <service>\n");
    printf ("     where option is one of the following:\n");
    printf ("       -d         show learned services\n");
    printf ("       -d -v      show learned services verbosely\n");
    printf ("       -p         connect to a local port rather than a service\n");
    printf ("       -H <node>  remote node name\n");
    printf ("       -R <port>  remote port name\n");
    printf ("       -r <port>  remote port name\n");
    printf ("       -Q         connect to a queued service\n");
    printf ("       -c         convert input CR to LF\n");
    printf ("       -b         convert input DEL to BS\n");
    printf ("       -l         convert output LF to VT\n");
    printf ("       -n <name>  Local port name\n");
    printf ("       -w <pass>  Service password (-w- will prompt)\n");
    printf ("       -q <char>  quit character\n");
    printf ("       -h         display this usage message\n");
    return 0;
}

int main(int argc, char *argv[])
{
    char msg[1024];
    char node[256] = {'\0'};
    char service[256] = {'\0'};
    char port[256] = {'\0'};
    char localport[256] = {'\0'};
    char password[256] = {'\0'};
    signed char opt;
    int verbose = 0;
    int crlf = 1;
    int bsdel = 0;
    int lfvt = 0;
    int show_services = 0;
    int use_port = 0;
    int is_queued = 0;
    int quit_char = 0x1d; // Ctrl-]

    if (argc == 1)
    {
	exit(usage(argv[0]));
    }

    // Set the default local port name
    if (ttyname(0)) strcpy(localport, ttyname(0));

    while ((opt=getopt(argc,argv,"dpcvhlbQWH:R:r:q:n:w:")) != EOF)
    {
	switch(opt)
	{
	case 'd':
	    show_services = 1;
	    break;

	case 'c':
	    crlf = 0;
	    break;

	case 'b':
	    bsdel = 1;
	    break;

	case 'l':
	    lfvt = 1;
	    break;

	case 'p':
	    use_port = 1;
	    break;

	case 'q':
	    if (optarg[0] == '0')
		quit_char = -1;
	    else
		quit_char = toupper(optarg[0]) - '@';
	    break;

	case 'Q':
	    is_queued = 1;
	    break;

	case 'v':
	    verbose = 1;
	    break;

	case 'H':
	    strcpy(node, optarg);
	    break;

	case 'w':
	    strcpy(password, optarg);
	    break;

	case 'W':
	    strcpy(password, "-");
	    break;

	case 'R':
	case 'r':
	    strcpy(port, optarg);
	    break;

	case 'n':
	    strcpy(localport, optarg);
	    break;

	default:
	    exit(usage(argv[0]));
	}
    }

    // Parameter is the remote service name, but there
    // must be only one.
    if (argc == optind+1)
    {
	strcpy(service, argv[argc-1]);
    }
    else
    {
	if (!show_services) exit(usage(argv[0]));
    }

    // This is just a bit like microcom...
    if (use_port)
    {
	do_use_port(service, quit_char, crlf, bsdel, lfvt);
	return 0;
    }

    // If password is "-" then prompt so the user doesn't have to
    // expose it on the command line.
    if (password[0] == '-' && password[1] == '\0' && isatty(0))
    {
	char *newpwd;
	newpwd = getpass("Password: ");
	if (newpwd == NULL || strlen(newpwd) > sizeof(password))
	{
	    printf("Password input cancelled");
	    return 0;
	}
	strcpy(password, newpwd);
    }

    // Open socket to latd
    if (!open_socket(false)) return 2;

    // -d just lists the available services
    if (show_services)
    {
	char verboseflag[1] = {verbose};

	// This is the same as latcp -d -l
	send_msg(latcp_socket, LATCP_SHOWSERVICE, verboseflag, 1);

	unsigned char *result;
	int len;
	int cmd;
	read_reply(latcp_socket, cmd, result, len);

	cout << result;

	delete[] result;
	return 0;
    }

    make_upper(node);
    make_upper(service);
    make_upper(port);

// Build up a terminal message for latd
    int ptr=0;
    msg[ptr++] = is_queued;
    add_string((unsigned char*)msg, &ptr, (unsigned char*)service);
    add_string((unsigned char*)msg, &ptr, (unsigned char*)node);
    add_string((unsigned char*)msg, &ptr, (unsigned char*)port);
    add_string((unsigned char*)msg, &ptr, (unsigned char*)localport);
    add_string((unsigned char*)msg, &ptr, (unsigned char*)password);

    send_msg(latcp_socket, LATCP_TERMINALSESSION, msg, ptr);

    unsigned char *result;
    int len;
    int cmd;
    int ret;
    ret = read_reply(latcp_socket, cmd, result, len);
    if (ret)
	return ret;

    // If the reply was good then go into terminal mode.
    terminal(latcp_socket, quit_char, crlf, bsdel, lfvt);

    shutdown(latcp_socket, 3);
    close(latcp_socket);
    return 0;
}


// Return 0 for success and -1 for failure
static int read_reply(int fd, int &cmd, unsigned char *&cmdbuf, int &len)
{
    unsigned char head[3];

    // Get the message header (cmd & length)
    if (read(fd, head, sizeof(head)) != 3)
	return -1; // Bad header

    len = head[1] * 256 + head[2];
    cmd = head[0];
    cmdbuf = new unsigned char[len];

    // Read the message buffer
    if (read(fd, cmdbuf, len) != len)
    {
	return -1; // Bad message
    }

    if (cmd == LATCP_ERRORMSG)
    {
	fprintf(stderr, "%s\n", cmdbuf);
	return -1;
    }

    return 0;
}

static bool open_socket(bool quiet)
{
    struct sockaddr_un sockaddr;

    latcp_socket = socket(AF_UNIX, SOCK_STREAM, PF_UNIX);
    if (latcp_socket == -1)
    {
	if (!quiet) perror("Can't create socket");
	return false; /* arggh ! */
    }

    strcpy(sockaddr.sun_path, LLOGIN_SOCKNAME);
    sockaddr.sun_family = AF_UNIX;
    if (connect(latcp_socket, (struct sockaddr *)&sockaddr, sizeof(sockaddr)))
    {
	if (!quiet) perror("Can't connect to latd");
	close(latcp_socket);
	return false;
    }

    unsigned char *result;
    int len;
    int cmd;

    // Send our version
    send_msg(latcp_socket, LATCP_VERSION, VERSION, strlen(VERSION)+1);
    read_reply(latcp_socket, cmd, result, len); // Read version number back

    return true;
}

static void make_upper(char *str)
{
    unsigned int i;

    for (i=0; i<strlen(str); i++)
    {
	str[i] = toupper(str[i]);
    }
}


static bool send_msg(int fd, int cmd, char *buf, int len)
{
    unsigned char outhead[3];

    outhead[0] = cmd;
    outhead[1] = len/256;
    outhead[2] = len%256;
    if (write(fd, outhead, 3) != 3) return false;
    if (write(fd, buf, len) != len) return false;

    return true;
}

// Pretend to be a terminal connected to a LAT service
static int terminal(int latfd, int endchar, int crlf, int bsdel, int lfvt)
{
    int termfd = STDIN_FILENO;
    struct termios old_term;
    struct termios new_term;

    tcgetattr(termfd, &old_term);
    new_term = old_term;

// Set local terminal characteristics
    new_term.c_iflag &= ~BRKINT;
    new_term.c_iflag |= IGNBRK;
    new_term.c_lflag &= ~ISIG;
    new_term.c_cc[VMIN] = 1;
    new_term.c_cc[VTIME] = 0;
    new_term.c_lflag &= ~ICANON;
    new_term.c_lflag &= ~(ECHO | ECHOCTL | ECHONL);
    tcsetattr(termfd, TCSANOW, &new_term);

    while(true)
    {
	char inbuf[1024];
	fd_set in_set;
	FD_ZERO(&in_set);
	FD_SET(termfd, &in_set);
	FD_SET(latfd, &in_set);

	if (select(FD_SETSIZE, &in_set, NULL, NULL, NULL) < 0)
	{
	    break;
	}

	// Read from keyboard
	if (FD_ISSET(termfd, &in_set))
	{
	    int len;
	    int i;

	    if (( (len=read(termfd, &inbuf, sizeof(inbuf))) < 1))
	    {
		break;
	    }

	    for (i=0; i<len; i++)
	    {
		if (endchar >0 && inbuf[i] == endchar)
		    goto quit;

		if (inbuf[i] == '\n' && crlf)
		    inbuf[i] = '\r';

		if (inbuf[i] == '\177' && bsdel)
		    inbuf[i] = '\010';
	    }
	    write(latfd, inbuf, len);
	}

	// Read from LAT socket. buffered.
	if (FD_ISSET(latfd, &in_set))
	{
	    int len;
	    if ( (len = read(latfd, &inbuf, sizeof(inbuf))) < 1)
		break;
	    else
	    {
		if (lfvt)
		{
		    for (int i=0; i<len; i++)
			if (inbuf[i] == '\n')
			    inbuf[i] = '\v';
		}
		write(termfd, inbuf, len);
	    }
	}
    }
 quit:
    // Reset terminal attributes
    tcsetattr(termfd, TCSANOW, &old_term);
    printf("\n");

    return 0;
}

static int do_use_port(char *portname, int quit_char, int crlf, int bsdel, int lfvt)
{
    int termfd;
    struct termios old_term;
    struct termios new_term;

    termfd = open(portname, O_RDWR);
    if (termfd < 0)
    {
	fprintf(stderr, "Cannot open device %s: %s\n", portname, strerror(errno));
	return -1;
    }

    tcgetattr(termfd, &old_term);
    new_term = old_term;

    // Set local terminal characteristics
    new_term.c_iflag &= ~(BRKINT | ICRNL);
    new_term.c_iflag |= IGNBRK;
    new_term.c_lflag &= ~ISIG;
    new_term.c_oflag &= ~OCRNL;
    new_term.c_cc[VMIN] = 1;
    new_term.c_cc[VTIME] = 0;
    new_term.c_lflag &= ~ICANON;
    new_term.c_lflag &= ~(ECHO | ECHOCTL | ECHONL);
    tcsetattr(termfd, TCSANOW, &new_term);

    // Be a terminal
    terminal(termfd, quit_char, crlf, bsdel, lfvt);

    // Reset terminal attributes
    tcsetattr(termfd, TCSANOW, &old_term);
    close(termfd);

    return 0;


}
