/******************************************************************************
    (c) 2000 Patrick Caulfield                 patrick@debian.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
******************************************************************************/
#include <mcheck.h>
#include <sys/types.h>
#include <sys/uio.h>
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
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <list>
#include <queue>
#include <string>
#include <map>
#include <iterator>
#include <strstream>

#include "lat.h"
#include "utils.h"
#include "session.h"
#include "connection.h"
#include "circuit.h"
#include "latcpcircuit.h"
#include "server.h"
#include "services.h"

static  void sigchild(int s);
static  void sigterm(int s);
static  void sighup(int s);

static void usage(char *prog, FILE *f)
{
    fprintf(f,"\n%s options:\n", prog);
    fprintf(f," -v        Verbose messages\n");
    fprintf(f," -h        Show this help text\n");
    fprintf(f," -d        Debug - don't do initial fork\n");
    fprintf(f," -t        Make rating static (don't check loadvg)\n");
    fprintf(f," -r<num>   Service rating (max if dynamic)\n");
    fprintf(f," -s<name>  Service name\n");
    fprintf(f," -c<num>   Circuit Timer in ms (default 80)\n");
    fprintf(f," -g<text>  Greeting text\n");
    fprintf(f," -i<name>  Interface name (Default to all ethernet)\n");
    fprintf(f," -l<type>  Logging type(s:syslog, e:stderr, m:mono)\n");
    fprintf(f," -V        Show version number\n\n");
}

/* Start Here */
int main(int argc, char *argv[])
{
    signed char opt;
    int  verbosity = 0;
    int  debug = 0;
    char log_char='l';
    char interface[44];
    int  circuit_timer = 80;
    int  rating = 12;
    char service[256];
    char greeting[256];
    int  static_rating = 0;
    char *interfaces[256];
    int num_interfaces = 0;

#ifdef DEBUG_MALLOC
    putenv("MALLOC_TRACE=/tmp/mtrace.log");
    mtrace();
#endif

    strcpy(greeting,  "A Linux box");
    interface[0] = '\0';
    memset(interfaces, 0, sizeof(interfaces));

    strcpy(service, (char *)LATServer::Instance()->get_local_node());
    
    // Deal with command-line arguments. Do these before the check for root
    // so we can check the version number and get help without being root.
    opterr = 0;
    optind = 0;
    while ((opt=getopt(argc,argv,"?vVhdl:r:s:t:g:i:c:")) != EOF)
    {
	switch(opt) 
	{
	case 'h': 
	    usage(argv[0], stdout);
	    exit(0);

	case '?':
	    usage(argv[0], stderr);
	    exit(0);

	case 'v':
	    verbosity++;
	    break;

	case 'd':
	    debug++;
	    break;

	case 'r':
	    rating = atoi(optarg);
	    break;

	case 'c':
	    circuit_timer = atoi(optarg);
	    if (circuit_timer == 0)
	    {
		usage(argv[0], stderr);
		exit(3);
	    }
	    break;

	case 'i':
	    interfaces[num_interfaces++] = optarg;
	    break;

	case 's':
	    strcpy(service, optarg);
	    break;

	case 'g':
	    strcpy(greeting, optarg);
	    break;

	case 't':
	    static_rating = 1;
	    break;

	case 'V':
	    printf("\nlatd version %s\n\n", VERSION);
	    exit(1);
	    break;
	    	    
	case 'l':
	    if (optarg[0] != 's' &&
		optarg[0] != 'm' &&
		optarg[0] != 'e')
	    {
		usage(argv[0], stderr);
		exit(2);
	    }
	    log_char = optarg[0];
	    break;
	}
    }

    // We need to be root from now on.
    if (getuid() != 0)
    {
	fprintf(stderr, "You need to be root to run this\n");
	exit(2);
    }

    // Make sure we were started by latcp
    if (getenv("LATCP_STARTED") == NULL)
    {
	fprintf(stderr, "\nlatd must be started with latcp -s\n");
	fprintf(stderr, "see the man page for more information.\n\n");
	exit(2);
    }

    
#ifndef NO_FORK
    if (!debug) // Also available at run-time
    {
	pid_t pid;
	switch ( pid=fork() )
	{
	case -1:
	    perror("server: can't fork");
	    exit(2);
	    
	case 0: // child
	    break;
	    
	default: // Parent.
	    if (verbosity > 1) printf("server: forked process %d\n", pid);
	    exit(0); 
	}
    
	// Detach ourself from the calling environment
	int devnull = open("/dev/null", O_RDWR);
	close(0);
	close(1);
	close(2);
	setsid();
	dup2(devnull, 0);
	dup2(devnull, 1);
	dup2(devnull, 2);
	chdir("/");
    }
#endif
    
    struct   sigaction siga;
    sigset_t ss;
  
    sigemptyset(&ss);
    siga.sa_handler=sigchild;
    siga.sa_mask  = ss;
    siga.sa_flags = 0;
    sigaction(SIGCHLD, &siga, NULL);

    // Make sure we shut down tidily
    siga.sa_handler=sigterm;
    sigaction(SIGTERM, &siga, NULL);

    siga.sa_handler=sighup;
    sigaction(SIGHUP, &siga, NULL);

    // Ignore these.
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
   
    openlog("latd", LOG_PID, LOG_DAEMON);

    // Go!
    LATServer *server = LATServer::Instance();
    server->init(static_rating, rating, service, greeting,
		 interfaces, verbosity, circuit_timer);
    server->run();
    
    return 0;
}


// Catch child process shutdown
static void sigchild(int s)
{
    int status, pid;

    // Make sure we reap all children
    do 
    { 
	pid = waitpid(-1, &status, WNOHANG); 
    }
    while (pid > 0);
}

// Catch termination signal
static void sigterm(int s)
{
    syslog(LOG_INFO, "SIGTERM caught, going down\n");
    LATServer *server = LATServer::Instance();
    server->shutdown();
}


// Catch hangup signal
// Resend service announcement.
static void sighup(int s)
{
    // Can't do it right now because alarm(0) cancels!
    alarm(1);
}


