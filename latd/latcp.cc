/******************************************************************************
    (c) 2000 Patrick Caulfield                 patrick@pandh.demon.co.uk

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
******************************************************************************/

// LAT Control Program (latcp)


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
#include <assert.h>

#include <list>
#include <queue>
#include <map>
#include <string>
#include <algo.h>
#include <iterator>
#include <string>
#include <strstream>

#include "latcp.h"
#include "utils.h"
#include "dn_endian.h"

static int latcp_socket;

static void make_upper(char *str);

bool read_reply(int fd, int &cmd, unsigned char *&cmdbuf, int &len);
bool send_msg(int fd, int cmd, char *buf, int len);
bool open_socket(bool);

// Command processing routines
void display(int argc, char *argv[]);
void add_service(int argc, char *argv[]);
void del_service(int argc, char *argv[]);
void set_responder(int onoff);
void shutdown();

int usage(char *cmd)
{
// TODO: trim this down to match reality

    printf ("Usage: latcp      {option}\n");
    printf ("     where option is one of the following:\n");
    printf ("       -s \n");
    printf ("       -h\n");
    printf ("       -A -a service [-i descript] [-o | -p ttylist]\n");
    printf ("       -A -v reserved_service\n");
    printf ("       -A -p tty -H rem_node {-R rem_port | -V rem_service} [-Q] [-wpass | -W]\n");
    printf ("       -A -p tty -O -V learned_service \n");
    printf ("               [-H rem_node [-R rem_port] ] [-wpass | -W]\n");
    printf ("       -p ttylist -a service\n");
    printf ("       -P ttylist -a service\n");
    printf ("       -D {-a service | -v reserved_service | -p ttylist}\n");
    printf ("       -i descript -a service\n");
    printf ("       -g list -a service\n");
    printf ("       -G list -a service\n");
    printf ("       -u list\n");
    printf ("       -U list\n");
    printf ("       -j\n");
    printf ("       -J\n");
    printf ("       -Y\n");
    printf ("       -x rating -a service\n");
    printf ("       -n node\n");
    printf ("       -m time\n");
#if 0
    printf ("       -e adaptor\n");
    printf ("       -E adaptor\n");
    printf ("       -c count\n");
#endif
    printf ("       -d [ [-l [-v learned_service] ] | -H rem_node | -C | -N | -S | \n");
    printf ("               -P [-p ttylist | -L | -I | -O] ]\n");
    printf ("       -z \n");
    printf ("       -r\n");

    return 2;
}

int main(int argc, char *argv[])
{

// Parse the command.
// because the args vary so much for each command I just check argv[1]
// for the command switch and the command processors call getopt themselves    

    if (argc == 1)
    {
	exit(usage(argv[0]));
    }

    if (argv[1][0] != '-')
    {
	exit(usage(argv[0]));
    }

    switch (argv[1][1])
    {
    case 'A':
	add_service(argc-1, &argv[1]);
	break;
    case 'D':
	del_service(argc-1, &argv[1]);
	break;
    case 'j':
	set_responder(1);
	break;
    case 'J':
	set_responder(0);
	break;
    case 'Y':
	printf("not yet done\n");
	break;
    case 'x':
	printf("not yet done\n");
	break;
    case 'n':
	printf("not yet done\n");
	break;
    case 'm':
	printf("not yet done\n");
	break;
    case 'd':
	display(argc-1, &argv[1]);
	break;
    case 'h':
	shutdown();
	break;
    case 'z':
	printf("not yet done\n");
	break;
    case 'r':
	printf("not yet done\n");
	break;
    case 'U':
	printf("not yet done\n");
	break;
    case 'u':
	printf("not yet done\n");
	break;
    default:
	exit(usage(argv[0]));
	break;
    } 
}


// Display latd characteristics or learned services
void display(int argc, char *argv[])
{   
    if (!open_socket(false)) return;

    // Just do the one command for the moment - dump service list
    char verboseflag[1] = {'\0'};
    send_msg(latcp_socket, LATCP_SHOWSERVICE, verboseflag, 1);

    unsigned char *result;
    int len;
    int cmd;
    read_reply(latcp_socket, cmd, result, len);

    cout << result;

    delete[] result;
   
}

// Enable/Disable the service responder
void set_responder(int onoff)
{
    if (!open_socket(false)) return;

    char flag[1];
    flag[0] = onoff;
    send_msg(latcp_socket, LATCP_SETRESPONDER, flag, 1);    
}

// Shutdown latd
void shutdown()
{
    if (!open_socket(false)) return;

    char dummy[1];
    send_msg(latcp_socket, LATCP_SHUTDOWN, dummy, 0);
}


void add_service(int argc, char *argv[])
{
    char name[255] = {'\0'};
    char ident[255] = {'\0'};
    char remport[255] = {'\0'};
    char localport[255] = {'\0'};
    char remnode[255] = {'\0'};
    char remservice[255] = {'\0'};
    char opt;
    bool got_service=false;
    bool got_port=false;
    int queued = 0;
    opterr = 0;
    optind = 0;

    while ((opt=getopt(argc,argv,"a:i:p:H:R:V:")) != EOF)
    {
	switch(opt) 
	{
	case 'a':
	    got_service=true;
	    strcpy(name, optarg);
	    break;

	case 'i':
	    got_service=true;
	    strcpy(ident, optarg);
	    break;

	case 'p':
	    got_port=true;
	    strcpy(localport, optarg);
	    break;

	case 'H':
	    got_port=true;
	    strcpy(remnode, optarg);
	    make_upper(remnode);
	    break;

	case 'R':
	    got_port=true;
	    strcpy(remport, optarg);
	    make_upper(remport);
	    break;

	case 'V':
	    got_port=true;
	    strcpy(remservice, optarg);
	    make_upper(remservice);
	    break;

	case 'Q':
	    got_port=true;
	    queued = 1;
	    break;

	default:
	    fprintf(stderr, "No more service switches defined yet\n");
	    exit(2);
	}    
    }

    if (!open_socket(false)) return;

    // Variables for ACK message
    unsigned char *result;
    int len;
    int cmd;

    if (got_service)
    {
	char message[520];
	int ptr = 0;
	add_string((unsigned char*)message, &ptr, (unsigned char*)name);
	add_string((unsigned char*)message, &ptr, (unsigned char*)ident);
	send_msg(latcp_socket, LATCP_ADDSERVICE, message, ptr);

	// Wait for ACK or error
	read_reply(latcp_socket, cmd, result, len);
	return;
    }

    if (got_port)
    {
	char message[520];
	int ptr = 0;
	add_string((unsigned char*)message, &ptr, (unsigned char*)remservice);
	add_string((unsigned char*)message, &ptr, (unsigned char*)remport);
	add_string((unsigned char*)message, &ptr, (unsigned char*)localport);
	add_string((unsigned char*)message, &ptr, (unsigned char*)remnode);
	message[ptr++] = queued;
	send_msg(latcp_socket, LATCP_ADDPORT, message, ptr);

	// Wait for ACK or error
	read_reply(latcp_socket, cmd, result, len);
	return;
    }

    fprintf(stderr, "Sorry, did you want me to do something??\n");
}

void del_service(int argc, char *argv[])
{
    char name[255] = {'\0'};
    char opt;
    opterr = 0;
    optind = 0;

    while ((opt=getopt(argc,argv,"a:")) != EOF)
    {
	switch(opt) 
	{
	case 'a':
	    strcpy(name, optarg);
	    break;
	default:
	    fprintf(stderr, "No more service switches defined yet\n");
	    exit(2);
	}    
    }

    if (!open_socket(false)) return;

    char message[520];
    int ptr = 0;
    add_string((unsigned char*)message, &ptr, (unsigned char*)name);
    send_msg(latcp_socket, LATCP_REMSERVICE, message, ptr);

    unsigned char *result;
    int len;
    int cmd;
    read_reply(latcp_socket, cmd, result, len);
}

bool send_msg(int fd, int cmd, char *buf, int len)
{
    unsigned char outhead[3];
    
    outhead[0] = cmd;
    outhead[1] = len/256;
    outhead[2] = len%256;
    write(fd, outhead, 3);
    write(fd, buf, len);

    // TODO Error checking.
    return true;
}


bool read_reply(int fd, int &cmd, unsigned char *&cmdbuf, int &len)
{
    unsigned char head[3];
    
    // Get the message header (cmd & length)
    if (read(fd, head, sizeof(head)) != 3)
	return false; // Bad header
    
    len = head[1] * 256 + head[2];
    cmd = head[0];
    cmdbuf = new unsigned char[len];

    // Read the message buffer
    if (read(fd, cmdbuf, len) != len)
    {
	return false; // Bad message
    }

    if (cmd == LATCP_ERRORMSG)
    {
	fprintf(stderr, "%s\n", cmdbuf);
	return false;
    }
    
    return true;
}

bool open_socket(bool quiet)
{
    struct sockaddr_un sockaddr;
    
    latcp_socket = socket(AF_UNIX, SOCK_STREAM, PF_UNIX);
    if (latcp_socket == -1)
    {	
	if (!quiet) perror("Can't create socket");
	return false; /* arggh ! */
    }

    strcpy(sockaddr.sun_path, LATCP_SOCKNAME);
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

/*
  Commands: (Taken from Tru64 Unix)

  -A			Add Service
  -D			Delete a service
  -j			Enable node agent status  (Responder)
  -J			Disable node agent status 
  -Y			Purge learned service names (apart from reserved ones)
  -x rating		Set rating
  -n node		Set node name
  -m time		Set multicast timer
  -d			Display LAT characteristics
  -d -l                 Display learned services
  -h			Halt
  -z			Zero counters
  -r			Set default values
  -U grouplist		Disable outgoing groups
  -u grouplist		Enables outgoing groups

  ...probably won't do these...
  -e			Add an adaptor
  -E			Remove an adaptor
  -c                      Set maximum number of learned services.

-----------------------------------

Node name:  BALTI
Multicast timer:        60 seconds
LAT version:  5         ECO:    2
Outgoing Port Groups:   0

Selected Interface Name(s):     tu0
LAT Protocol is active
Agent Status: Disabled
Maximum Number of Learned Services: 100


Service information
        Service name:   BALTI
        Service ID:     Digital UNIX Version V4.0 LAT SERVICE
        Rating:         Dynamic         127
        Groups:         0


*/
