/******************************************************************************
    (c) 2002      P.J. Caulfield          patrick@debian.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 ******************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <netdnet/dn.h>
#include <netdnet/dnetdb.h>
#include "cterm.h"
#include "dn_endian.h"
#include "dnlogin.h"

/* The global state */
int termfd = -1;
int exit_char = 035; /* Default to ^] */
int finished = 0;    /* terminate mainloop */

int debug = 0;

static int mainloop(void)
{
    while (!finished)
    {
	char inbuf[1024];

	fd_set in_set;
	FD_ZERO(&in_set);
	FD_SET(termfd, &in_set);
	FD_SET(found_getsockfd(), &in_set);

	if (select(FD_SETSIZE, &in_set, NULL, NULL, NULL) < 0)
	{
	    break;
	}

	/* Read from keyboard */
	if (FD_ISSET(termfd, &in_set))
	{
	    int len;

	    if ( (len=read(termfd, inbuf, sizeof(inbuf))) <= 0)
	    {
		perror("read tty");
		break;
	    }
	    tty_process_terminal(inbuf, len);
	}

	if (found_read() == -1)
	    break;
    }
    return 0;
}

static void set_exit_char(char *string)
{
    int newchar = 0;

    if (string[0] == '^')
    {
	newchar = toupper(string[1]) - '@';
    }
    else
	newchar = strtol(string, NULL, 0);	// Just a number

    // Make sure it's resaonable
    if (newchar > 0 && newchar < 256)
	exit_char = newchar;

}

static void usage(char *prog, FILE * f)
{
    fprintf(f, "\nUsage: %s [OPTIONS] node\n\n", prog);

    fprintf(f, "\nOptions:\n");
    fprintf(f, "  -? -h        display this help message\n");
    fprintf(f, "  -V           show version number\n");
    fprintf(f, "  -e <char>    set exit char\n");
    fprintf(f, "  -d           debug information\n");

    fprintf(f, "\n");
}


int main(int argc, char *argv[])
{
    struct sigaction sa;
    sigset_t ss;
    int opt;
    char *nodename;

    // Deal with command-line arguments.
    opterr = 0;
    optind = 0;
    while ((opt = getopt(argc, argv, "?Vhdte:")) != EOF)
    {
	switch (opt)
	{
	case 'h':
	    usage(argv[0], stdout);
	    exit(0);

	case '?':
	    usage(argv[0], stderr);
	    exit(0);

	case 'V':
	    printf("\ndnlogin from dnprogs version %s\n\n", VERSION);
	    exit(1);
	    break;

	case 'e':
	    set_exit_char(optarg);
	    break;

	case 'd':
	    debug++;
	    break;
	}
    }

    if (optind >= argc)
    {
	usage(argv[0], stderr);
	exit(2);
    }

    send_input = cterm_send_input;
    if (found_setup_link(argv[optind], DNOBJECT_CTERM, cterm_process_network) == 0)
    {
	tty_setup("/dev/tty", 1);
	mainloop();
	tty_setup(NULL, 0);
    }
    else
    {
	return 2;
    }

    return 0;
}
