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

/* Input state buffers & variables */
static unsigned char terminators[32];
static char input_buf[1024];
static int  input_len;
static char prompt_buf[1024];
static char prompt_len;
static char esc_buf[132];
static int  esc_len;
static int  max_read_len = sizeof(input_buf);
static int  echo = 1;

/* Output processor */
int (*send_input)(char *buf, int len, int flags);

/* Raw write to terminal */
int tty_write(char *buf, int len)
{
    return (write(termfd, buf, len) == len);
}

void tty_set_terminators(char *buf, int len)
{
    memset(terminators, 0, sizeof(terminators));
    memcpy(terminators, buf, len);
}

void tty_start_read(char *prompt, int len, int promptlen)
{
    write(termfd, prompt, len);

    /* Save the actual prompt in one buffer and the prefilled
       data in th input buffer */
    memcpy(prompt_buf, prompt, promptlen);
    prompt_len = promptlen;

    memcpy(input_buf, prompt+promptlen, len-promptlen);
    input_len = len-promptlen;
}

void tty_set_timeout(unsigned short to)
{
    // TODO:
}

void tty_set_maxlen(unsigned short len)
{
    max_read_len = len;
}

/* Set/Reset the local TTY mode */
int tty_setup(char *name, int setup)
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

static short is_terminator(char c)
{
    short termind, msk, aux;

    termind = c / 8;
    aux = c - (termind * 8);
    msk = (1 << aux);

    if (terminators[termind] && msk)
    {
	return 1;
    }
    return 0;
}

/* Input from keyboard */
int tty_process_terminal(char *buf, int len)
{
    int i;

    for (i=0; i<len; i++)
    {
	if (buf[i] == exit_char)
	{
	    finished = 1;
	    return 0;
	}

	if (echo)
	    write(termfd, &buf[i], 1);

	if (is_terminator(buf[i]))
	{
	    send_input(&buf[i], 1, 0);
	    return 0;
	}

	// TODO: process line-editting keys & flags
	input_buf[input_len++] = buf[i];
	if (input_len == max_read_len)
	{
	    send_input(input_buf, input_len, 0);
	}

	// TODO: Loads more.
    }
    return 0;
}
