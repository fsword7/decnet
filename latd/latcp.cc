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

#include "latcp.h"
#include "utils.h"
#include "dn_endian.h"

static int latcp_socket;

static void make_upper(char *str);

int  read_reply(int fd, int &cmd, unsigned char *&cmdbuf, int &len);
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
void purge_services();
void start_latd(int argc, char *argv[]);
void set_rating(int argc, char *argv[]);
void set_ident(int argc, char *argv[]);
void set_server_groups(int argc, char *argv[]);
void set_user_groups(int argc, char *argv[]);
void set_node (char *);


// Misc utility routines
static void make_bitmap(char *bitmap, char *cmdline);


int usage(char *cmd)
{
    printf ("Usage: latcp      {option}\n");
    printf ("     where option is one of the following:\n");
    printf ("       -s [<latd args>]\n");
    printf ("       -h\n");
    printf ("       -A -a service [-i descript] [-r rating] [-s]\n");
    printf ("       -A -p tty -H rem_node {-R rem_port | -V rem_service} [-Q] [-wpass | -W]\n");
    printf ("       -D {-a service | -p tty}\n");
    printf ("       -i descript -a service\n");
    printf ("       -g list\n");
    printf ("       -G list\n");
    printf ("       -u list\n");
    printf ("       -U list\n");
    printf ("       -j\n");
    printf ("       -J\n");
    printf ("       -Y\n");
    printf ("       -x rating [-s] -a service\n");
    printf ("       -n node\n");
    printf ("       -r retransmit limit\n");
    printf ("       -m multicast timer (100ths/sec)\n");
    printf ("       -k keepalive timer (seconds)\n");
    printf ("       -d [ [-l [-v] ] ]\n");
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
	purge_services();
	break;
    case 'n':
	set_node(argv[2]);
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
    case 'x':
	set_rating(argc, argv);
	break;
    case 'i':
	set_ident(argc, argv);
	break;
    case 'z':
	printf("counters not yet done\n");
	break;
    case 'U':
	set_user_groups(argc, argv);
	break;
    case 'u':
	set_user_groups(argc, argv);
	break;
    case 'g':
	set_server_groups(argc, argv);
	break;
    case 'G':
	set_server_groups(argc, argv);
	break;
    default:
	exit(usage(argv[0]));
	break;
    } 
}


// Display latd characteristics or learned services
void display(int argc, char *argv[])
{   
    char verboseflag[1] = {'\0'};
    char opt;
    bool show_services = false;
    
    if (!open_socket(false)) return;

    while ((opt=getopt(argc,argv,"lv")) != EOF)
    {
	switch(opt) 
	{
	case 'l':
	    show_services=true;
	    break;

	case 'v':
	    verboseflag[0] = 1;
	    break;

	default:
	    fprintf(stderr, "only -v or -l valid with -d flag\n");
	    exit(2);
	}
    }


    if (show_services)
    {
	send_msg(latcp_socket, LATCP_SHOWSERVICE, verboseflag, 1);
    }
    else
    {
	send_msg(latcp_socket, LATCP_SHOWCHAR, verboseflag, 1);
    }

    unsigned char *result;
    int len;
    int cmd;
    read_reply(latcp_socket, cmd, result, len);

    cout << result;

    delete[] result;  
}


// Change the rating of a service
void set_rating(int argc, char *argv[])
{
    int new_rating = 0;
    bool static_rating = false;
    char service[256];
    char opt;
    
    if (!open_socket(false)) return;

    while ((opt=getopt(argc,argv,"x:sa:")) != EOF)
    {
	switch(opt) 
	{
	case 'x':
	    new_rating = atoi(optarg);
	    break;

	case 's':
	    static_rating = true;
	    break;

	case 'a':
	    strcpy(service, optarg);
	    break;

	default:
	    fprintf(stderr, "only valid with -a and -x flags\n");
	    exit(2);
	}
    }

    if (!new_rating)
    {
	fprintf(stderr, "Invalid rating\n");
	exit(2);
    }

    make_upper(service);
    
    char message[520];
    int ptr = 2;
    message[0] = (int)static_rating;
    message[1] = new_rating;
    add_string((unsigned char*)message, &ptr, (unsigned char*)service);
    send_msg(latcp_socket, LATCP_SETRATING, message, ptr);

    // Wait for ACK or error
    unsigned char *result;
    int len;
    int cmd;

    exit (read_reply(latcp_socket, cmd, result, len));
    
}


// Change the ident of a service
void set_ident(int argc, char *argv[])
{
    char service[256];
    char ident[256];
    char opt;
    
    if (!open_socket(false)) return;

    while ((opt=getopt(argc,argv,"i:a:")) != EOF)
    {
	switch(opt) 
	{
	case 'a':
	    strcpy(service, optarg);
	    break;
	case 'i':
	    strcpy(ident, optarg);
	    break;

	default:
	    fprintf(stderr, "must have both -i and -a flags\n");
	    exit(2);
	}
    }

    make_upper(service);
    
    char message[1024];
    int ptr = 0;
    add_string((unsigned char*)message, &ptr, (unsigned char*)service);
    add_string((unsigned char*)message, &ptr, (unsigned char*)ident);
    send_msg(latcp_socket, LATCP_SETIDENT, message, ptr);

    // Wait for ACK or error
    unsigned char *result;
    int len;
    int cmd;
    exit(read_reply(latcp_socket, cmd, result, len));
}


void set_server_groups(int argc, char *argv[])
{
    char opt;
    int  cmd;
    char groups[256];
    
    while ((opt=getopt(argc,argv,"G:g:")) != EOF)
    {
	switch(opt) 
	{
	case 'G':
	    cmd = LATCP_SETSERVERGROUPS;
	    strcpy(groups, optarg);
	    break;

	case 'g':
	    cmd = LATCP_UNSETSERVERGROUPS;
	    strcpy(groups, optarg);
	    break;

	default:
	    fprintf(stderr, "Just G or g please\n");
	    exit(2);
	}
    }

    if (!open_socket(false)) return;

    char bitmap[32]; // 256 bits
    make_bitmap(bitmap, groups);
    
    send_msg(latcp_socket, cmd, bitmap, 32);

    // Wait for ACK or error
    unsigned char *result;
    int len;
    exit(read_reply(latcp_socket, cmd, result, len));

}

void set_user_groups(int argc, char *argv[])
{
    char opt;
    int  cmd;
    char groups[256];
    
    while ((opt=getopt(argc,argv,"u:U:")) != EOF)
    {
	switch(opt) 
	{
	case 'U':
	    cmd = LATCP_SETUSERGROUPS;
	    strcpy(groups, optarg);
	    break;

	case 'u':
	    cmd = LATCP_UNSETUSERGROUPS;
	    strcpy(groups, optarg);
	    break;

	default:
	    fprintf(stderr, "Just U or u please\n");
	    exit(2);
	}
    }

    if (!open_socket(false)) return;

    char bitmap[32]; // 256 bits
    make_bitmap(bitmap, groups);
    
    send_msg(latcp_socket, cmd, bitmap, 32);

    // Wait for ACK or error
    unsigned char *result;
    int len;
    exit(read_reply(latcp_socket, cmd, result, len));

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

// Purge learned services
void purge_services()
{
    if (!open_socket(false)) return;

    char dummy[1];
    send_msg(latcp_socket, LATCP_PURGE, dummy, 0);
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

void set_node(char *name)
{
    if (!open_socket(false)) return;

    make_upper(name);
    
    char message[520];
    int ptr = 0;
    add_string((unsigned char*)message, &ptr, (unsigned char*)name);
    
    send_msg(latcp_socket, LATCP_SETNODENAME, message, ptr);
    return;
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
    bool static_rating=false;
    int  rating = 0;
    int  queued = 0;
    
    opterr = 0;
    optind = 0;

    while ((opt=getopt(argc,argv,"a:i:p:H:R:V:r:s")) != EOF)
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

	case 's':
	    static_rating = true;
	    break;

	case 'r':
	    rating = atoi(optarg);
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
	if (name[0] == '\0')
	{
	    fprintf(stderr, "No name for new service\n");
	    exit(2);
	}

	
	char message[520];
	int ptr = 2;
	message[0] = (int)static_rating;
	message[1] = rating;
	add_string((unsigned char*)message, &ptr, (unsigned char*)name);
	add_string((unsigned char*)message, &ptr, (unsigned char*)ident);
	send_msg(latcp_socket, LATCP_ADDSERVICE, message, ptr);

	// Wait for ACK or error
	exit(read_reply(latcp_socket, cmd, result, len));
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
	exit(read_reply(latcp_socket, cmd, result, len));
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
    exit(read_reply(latcp_socket, cmd, result, len));
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
	char *newargv[argc+1];
	char *newenv[4];
	int   i;
	char  latcp_proc[PATH_MAX];
	char  latcp_bin[PATH_MAX];
	char  latcp_env[PATH_MAX+7];

// This is VERY Linux specific and need /proc mounted. 
// we get the full path of the current executable by doing a readlink.
// /proc/<pid>/exe

	sprintf(latcp_proc, "/proc/%d/exe", getpid());
	if ( (i=readlink(latcp_proc, latcp_bin, sizeof(latcp_bin))) == -1)
	{
	    fprintf(stderr, "readlink in /proc failed. Make sure the the proc filesystem is mounted on /proc\n");
	    exit(2);
	}
	sprintf(latcp_env, "LATCP=%s", latcp_bin);

	newargv[0] = latd_bin;
	newargv[1] = NULL;
	newenv[0] = "PATH=/bin:/usr/bin:/sbin:/usr/sbin";
	newenv[1] = "LATCP_STARTED=true"; // Tell latd it was started by us.
	newenv[2] = latcp_env;
	newenv[3] = NULL;

	switch(fork())
	{
	case 1: //Error
	    perror("fork failed");
	    return;
	case 0: // Child
	    // Start latd with our args (after the "-s")
	    for (i=2; i<argc; i++)
		newargv[i-1] = argv[i];
	    newargv[i-1] = NULL;

	    execve(latd_bin, newargv, newenv);
	    perror("exec of latd failed");
	    break;

	default: //Parent
	    {
		// Wait for latd to start up
		int count = 0;
		while (!open_socket(true) && count < 10)
		{
		    sleep(1);
		    count++;
		}
		if (count >= 10)
		{
		    fprintf(stderr, "latd did not start\n");
		    exit(2);
		}
		
		
		// Run startup script if there is one.
		if (!stat("/etc/latd.conf", &st))
		{
		    pid_t shell_pid;
		    switch ( (shell_pid=fork()) )
		    {
		    case 0: // Child
			newargv[0] = "/bin/sh";
			newargv[1] = "/etc/latd.conf";
			newargv[2] = NULL;
			execve("/bin/sh", newargv, newenv);
			perror("exec of /bin/sh failed");
			exit(0);
			
		    case -1:
			perror("Fork failed");
			exit(0);
			
		    default: // Parent. Wait for child to finish
			waitpid(shell_pid, NULL, 0);
		    }
		}
		// OK, latd has started and we have run the startup script.
		// Now "unlock" latd. ie tell it we have finished initialisation
		char dummy[1];
		send_msg(latcp_socket, LATCP_UNLOCK, dummy, 0);
		printf("LAT Started\n");
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
    if (write(fd, outhead, 3) != 3) return false;
    if (write(fd, buf, len) != len) return false;

    return true;
}


// Return 0 for success and -1 for failure
int read_reply(int fd, int &cmd, unsigned char *&cmdbuf, int &len)
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

static inline void set_in_bitmap(char *bits, int entry)
{

    if (entry < 256)
    {
	unsigned int  intnum;
	unsigned int  bitnum;
    
	intnum = entry / 8;
	bitnum = entry % 8;
	bits[intnum] |= 1<<bitnum;
    }
}

static void make_bitmap(char *bitmap, char *cmdline)
{
    int   firstnum;
    int   secondnum;
    int   i;
    bool  finished = false;
    char  delimchar;
    char* delimiter;

    memset(bitmap, 0, 32);
    delimiter = strpbrk(cmdline, ",-");
    
    do
    {
	if (delimiter == NULL)
	{
	    delimchar = ',';
	}
	else
	{
	    delimchar = delimiter[0];
	    delimiter[0] = '\0';
	}
	firstnum = atoi(cmdline);
	if (delimiter != NULL) delimiter[0] = delimchar;
	
	/* Found a comma -- mark the number preceding it as read */	
	if ((delimchar == ',') || (delimiter == NULL))
	{
	    set_in_bitmap(bitmap, firstnum);
	}
	
	/* Found a hyphen -- mark the range as read */
	if (delimchar == '-')
	{
	    cmdline = delimiter+1;
	    delimiter = strpbrk(cmdline, ",");

	    if (delimiter != NULL)
	    {
		delimchar = delimiter[0];
		delimiter[0] = '\0';
	    }
	    secondnum = atoi(cmdline);
	    if (delimiter != NULL) delimiter[0] = delimchar;
	    
	    for (i=firstnum; i<=secondnum; i++)
	    {
		set_in_bitmap(bitmap, i);
	    }
	}
	if (delimiter == NULL) finished = true;
	
	cmdline = delimiter+1;
	if (delimiter != NULL) delimiter = strpbrk(cmdline, ",-");
    } while (!finished);
}
