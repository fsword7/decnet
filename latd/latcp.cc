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
void set_multicast(int);
void set_retransmit(int);
void set_keepalive(int);
void set_responder(int onoff);
void shutdown();
void start_latd(int argc, char *argv[]);

int usage(char *cmd)
{
// TODO: trim this down to match reality

    printf ("Usage: latcp      {option}\n");
    printf ("     where option is one of the following:\n");
    printf ("       -s [<latd args>]\n");
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
    printf ("       -r retransmit limit\n");
    printf ("       -m multicast timer (100ths/sec)\n");    
    printf ("       -k keepalive timer (seconds)\n");
#if 0
    printf ("       -e adaptor\n");
    printf ("       -E adaptor\n");
    printf ("       -c count\n");
#endif
    printf ("       -d [ [-l [-v learned_service] ] | -H rem_node | -C | -N | -S | \n");
    printf ("               -P [-p ttylist | -L | -I | -O] ]\n");
    printf ("       -z \n");

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
	if (argv[2])
	    set_multicast(atoi(argv[2]));
	else
	    exit(usage(argv[0]));
	break;
    case 'r':
        if (argv[2])
            set_retransmit(atoi(argv[2]));
        else
            exit(usage(argv[0]));
        break;
    case 'k':
        if (argv[2])
            set_keepalive(atoi(argv[2]));
        else
            exit(usage(argv[0]));
        break;
    case 'd':
	display(argc-1, &argv[1]);
	break;
    case 'h':
	shutdown();
	break;
    case 's':
	start_latd(argc, argv);
	break;
    case 'z':
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
    printf("LAT stopped\n");
}

void set_multicast(int newtime)
{
    if (newtime == 0 || newtime > 32767)
    {
	fprintf(stderr, "invalid multicast time\n");
	return;
    }
    if (!open_socket(false)) return;

    send_msg(latcp_socket, LATCP_SETMULTICAST, (char *)&newtime, sizeof(int));
}

void set_retransmit(int newlim)
{
    if (newlim == 0 || newlim > 32767)
    {
        fprintf(stderr, "invalid retransmit limit\n");
        return;
    }
    if (!open_socket(false)) return;

    send_msg(latcp_socket, LATCP_SETRETRANS, (char *)&newlim, sizeof(int));
}

void set_keepalive(int newtime)
{
    if (newtime == 0 || newtime > 32767)
    {
	fprintf(stderr, "invalid keepalive timer\n");
	return;
    }
    if (!open_socket(false)) return;

    send_msg(latcp_socket, LATCP_SETKEEPALIVE, (char *)&newtime, sizeof(int));
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
	    if (strncmp(localport, "/dev/lat/", 9) != 0)
	    {
		fprintf(stderr, "Local port name must start /dev/lat\n");
		return;
	    }
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
    bool got_service = false;
    bool got_port = false;
    opterr = 0;
    optind = 0;

    while ((opt=getopt(argc,argv,"a:p:")) != EOF)
    {
	switch(opt) 
	{
	case 'a':
	    got_service = true;
	    strcpy(name, optarg);
	    break;
	    
	case 'p':
	    got_port = true;
	    strcpy(name, optarg);
	    break;

	default:
	    fprintf(stderr, "No more service switches defined yet\n");
	    exit(2);
	}    
    }

    if ((got_port && got_service) ||
	(!got_port && !got_service))
    {
	fprintf(stderr, "Either -a or -p can be specified but not both\n");
	return;
    }

    
    if (!open_socket(false)) return;

    char message[520];
    int ptr = 0;
    add_string((unsigned char*)message, &ptr, (unsigned char*)name);
    if (got_service)
	send_msg(latcp_socket, LATCP_REMSERVICE, message, ptr);
    else
	send_msg(latcp_socket, LATCP_REMPORT, message, ptr);
    
    unsigned char *result;
    int len;
    int cmd;
    read_reply(latcp_socket, cmd, result, len);
}

// Start latd & run startup script.
void start_latd(int argc, char *argv[])
{
    if (getuid() != 0)
    {
	fprintf(stderr, "You must be root to start latd\n");
	return;
    }

    // If we can connect to LATD than it's already running
    if (open_socket(true))
    {
	fprintf(stderr, "LAT is already running\n");
	return;
    }
    
    // Look for latd in well-known places
    struct stat st;
    char *latd_bin = NULL;
    char *latd_path = NULL;

    if (!stat("/usr/sbin/latd", &st))
    {
	latd_bin = "/usr/sbin/latd";
	latd_path = "/usr/sbin";
    }
    else if (!stat("/usr/local/sbin/latd", &st))
    {
	latd_bin = "/usr/local/sbin/latd";
	latd_path = "/usr/local/sbin";
    }
    else
    {
	char *name = (char *)malloc(strlen(argv[0])+1);
	char *path = (char *)malloc(strlen(argv[0])+1);
	strcpy(name, argv[0]);

	char *slash = rindex(name, '/');
	if (slash)
	{
	    *slash='\0';
	    strcpy(path, name);
	    strcat(name, "/latd");
	    if (!stat(name, &st))
	    {
		latd_bin = name;
		latd_path = path;
	    }
	}
    }

    // Did we find it?
    if (latd_bin)
    {
	char  newpath[1024];
	char *newargv[argc+1];
	char *newenv[argc+1];
	int   i;

	// Make a minimal path including wherever latd is.
	sprintf(newpath, "PATH=/bin:/usr/bin:/sbin:/usr/sbin:%s", latd_path);
	newargv[0] = latd_bin;
	newargv[1] = NULL;
	newenv[0] = newpath;
	newenv[1] = NULL;

	switch(fork())
	{
	case 1: //Error
	    perror("fork failed");
	    return;
	case 0: // Child
	    // Start latd with out args (after the "-s")
	    for (i=2; i<argc; i++)
		newargv[i-1] = argv[i];
	    newargv[i-1] = NULL;

	    execve(latd_bin, newargv, NULL);
	    perror("exec of latd failed");
	    break;

	default: //Parent
	    // Run startup script if there is one.
	    sleep(1);
	    printf("LAT started\n");
	    if (!stat("/etc/latd.conf", &st))
	    {
		newargv[0] = "/bin/sh";
		newargv[1] = "/etc/latd.conf";
		newargv[2] = NULL;
		execve("/bin/sh", newargv, newenv);
		perror("exec of /bin/sh failed");
	    }
	    break;
	}
    }
    else
    {
	fprintf(stderr, "cannot find latd\n");
	return;
    }
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
