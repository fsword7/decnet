/******************************************************************************
    (c) 2001 Patrick Caulfield                 patrick@debian.org

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
static int terminal(int latfd, int, int, int);

static int usage(char *cmd)
{
    printf ("Usage: llogin [<option>] <service>\n");
    printf ("     where option is one of the following:\n");
    printf ("       -d         show learned services\n");
    printf ("       -p         connect to a local port rather than a service\n");
    printf ("       -H <node>  remote node name\n");
    printf ("       -R <port>  remote port name\n");
    printf ("       -c         convert CR to LF\n");
    printf ("       -b         convert DEL to BS\n");
    printf ("       -q <char>  quit character\n");
    printf ("       -h         display this usage message\n");
    return 0;
}

int main(int argc, char *argv[])
{
    char msg[1024];
    char node[256];
    char service[256];
    char port[256];
    signed char opt;
    int verbose = 0;
    int crlf = 0;
    int bsdel = 0;
    int show_services = 0;
    int use_port = 0;
    int is_queued = 0;
    int quit_char = 0;

    if (argc == 1)
    {
	exit(usage(argv[0]));
    }  

    while ((opt=getopt(argc,argv,"dpcvhbQH:R:q:")) != EOF)
    {
	switch(opt) 
	{
	case 'd':
	    show_services = 1;
	    break;

	case 'c':
	    crlf = 1;
	    break;

	case 'b':
	    bsdel = 1;
	    break;

	case 'p':
	    use_port = 1;
	    break;

	case 'q':
	    if (optarg[0] == '0')
		quit_char = -1;
	    else
		quit_char = toupper(optarg[0]) - 'A' + 1;
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

	case 'R':
	    strcpy(port, optarg);
	    break;

	default:
	    exit(usage(argv[0]));
	}
    }
    
    if (optind < argc)
    {
	strcpy(service, argv[argc-1]);
    }
    else
    {
	if (!show_services) exit(usage(argv[0]));
    }

    if (!open_socket(false)) return 2;


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
    
// This is just a bit like microcom...
    if (use_port)
    {
//	do_use_port(service)
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

    send_msg(latcp_socket, LATCP_TERMINALSESSION, msg, ptr);

    unsigned char *result;
    int len;
    int cmd;
    int ret;
    ret = read_reply(latcp_socket, cmd, result, len);
    if (ret) return ret;

    // If the reply was good then go into terminal mode.
    terminal(latcp_socket, quit_char, crlf, bsdel);
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

// TODO: Make this nicer and more robust.
static int terminal(int latfd, int endchar, int crlf, int bsdel)
{
    int termfd = STDIN_FILENO;
    bool done = false;
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

    while(!done)
    {
	char inchar;
	char inbuf[1024];
	fd_set in_set;
	FD_ZERO(&in_set);
	FD_SET(termfd, &in_set);
	FD_SET(latfd, &in_set);

	if (select(FD_SETSIZE, &in_set, NULL, NULL, NULL) < 0)
	{
	    break;
	}

	// Read from keyboard. One at a time
	if (FD_ISSET(termfd, &in_set))
	{
	    if ((read(termfd, &inchar, 1) < 1) ||
		(endchar>0 && inchar == endchar))
	    {
		break;
	    }

	    if (inchar == '\n' && crlf)
		inchar = '\r';

	    if (inchar == '\177' && bsdel)
		inchar = '\010';

	    write(latfd, &inchar, 1);

	}

	// Read from LAT socket. buffered.
	if (FD_ISSET(latfd, &in_set))
	{
	    int len;
	    if ( (len = read(latfd, &inbuf, sizeof(inbuf))) < 1)
		break;
	    else
		write(termfd, inbuf, len);
	}
    }
    
    // Reset terminal attributes
    tcsetattr(termfd, TCSANOW, &old_term);

    return 0;
}
