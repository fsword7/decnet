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
static int termfd = -1;
static int exit_char = 035; /* Default to ^] */
static int finished = 0;    /* terminate mainloop */
int debug = 0;


/* Input state buffers & variables */
static unsigned char terminators[32];
static char input_buf[1024];
static int  input_len;
static char prompt_buf[1024];
static char prompt_len;
static char esc_buf[132];
static int  esc_len;


/* Set/Reset the local TTY mode */
static int setup_tty(char *name, int setup)
{
    struct termios new_term;
    static struct termios old_term;

    if (setup)
    {
	termfd = open(name, O_RDWR);

	tcgetattr(termfd, &old_term);
	new_term = old_term;

	new_term.c_iflag &= ~BRKINT;
	new_term.c_iflag |= IGNBRK;
	new_term.c_lflag &= ~ISIG;
	new_term.c_cc[VMIN] = 1;
	new_term.c_cc[VTIME] = 0;
	new_term.c_lflag &= ~ICANON;
	new_term.c_lflag &= ~(ECHO | ECHOCTL | ECHONL);
	tcsetattr(termfd, TCSANOW, &new_term);
    }
    else
    {
	tcsetattr(termfd, TCSANOW, &old_term);
	close(termfd);
    }

    return 0;
}
static const char *hosttype[] = {
    "RT-11",
    "RSTS/E",
    "RSX-11S",
    "RSX-11M",
    "RSX-11D",
    "IAS",
    "VAX/VMS",
    "TOPS-20",
    "TOPS-10",
    "OS8",
    "RTS-8",
    "RSX-11M+",
    "??13", "??14", "??15", "??16", "??17",
    "Ultrix-32",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "Unix-dni"
};


/* Input from keyboard */
static int process_terminal(char *buf, int len)
{
    if (buf[0] == exit_char)
    {
	finished = 1;
	return 0;
    }

    /* TODO: */

    return -1;
}

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
	    process_terminal(inbuf, len);
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
    fprintf(f, "\nUSAGE: %s [OPTIONS] node\n\n", prog);

    fprintf(f, "\nOptions:\n");
    fprintf(f, "  -? -h        display this help message\n");
    fprintf(f, "  -V           show version number\n");
    fprintf(f, "  -e <char>    set escape char\n");
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

    if (found_setup_link(argv[optind],DNOBJECT_CTERM) == 0)
    {
	setup_tty("/dev/tty", 1);
	mainloop();
	setup_tty(NULL, 0);
    }
    else
    {
	return 2;
    }

    return 0;
}
