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
#include "dn_endian.h"
#include "dnlogin.h"
#include "tty.h"

/* The global state */
extern int termfd;
extern int exit_char;
extern int finished;

/* Input state buffers & variables */
static unsigned char terminators[32];
static char rahead_buf[128];
static int  rahead_len=0;
static char input_buf[1024];
static int  input_len=0;
static int  input_pos=0;
static char prompt_buf[1024];
static char prompt_len=0;
static char esc_buf[132];
static int  esc_len=0;
static int  max_read_len = sizeof(input_buf);
static int  echo = 1;
static int  reading = 0;
static int  insert_mode = 0;

/* Output processor */
int (*send_input)(char *buf, int len, int flags);

/* Raw write to terminal */
int tty_write(char *buf, int len)
{
    int i;

    for (i=0; i<len; i++)
    {
	if (buf[i] == '\r')
	    write(termfd, "\r\n", 2);
	else
	    write(termfd, &buf[i], 1);
    }
    return len;
}

void tty_set_noecho()
{
    echo = 0;
}


void tty_set_terminators(char *buf, int len)
{
    memset(terminators, 0, sizeof(terminators));
    memcpy(terminators, buf, len);
}

void tty_start_read(char *prompt, int len, int promptlen)
{
    if (debug & 4)
	fprintf(stderr, "TTY start_read promptlen = %d, maxlen=%d\n",
		promptlen, len);
    if (promptlen) write(termfd, prompt, promptlen);

    /* Save the actual prompt in one buffer and the prefilled
       data in the input buffer */
    memcpy(prompt_buf, prompt, promptlen);
    prompt_len = promptlen;

    memcpy(input_buf, prompt+promptlen, len);
    input_len = 0;
    input_pos = input_len;
    max_read_len = len;

    /* Now add in any typeahead
       TODO: Are there flags to disable this? */
    if (rahead_len)
    {
	int copylen = rahead_len;

	fprintf(stderr, "PJC: readahead = %d bytes\n", rahead_len);
	/* Don't overflow the input buffer */
	if (input_len + copylen > sizeof(input_buf))
	    copylen = sizeof(input_buf)-input_len;

	memcpy(input_buf+input_len, rahead_buf, copylen);
	input_len += copylen;
    }
    reading = 1;
}

void tty_set_timeout(unsigned short to)
{
    // TODO:
}

void tty_clear_typeahead()
{
    rahead_len = 0;
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

/* Erase to end of line */
static void erase_eol(void)
{
    write(termfd, "\033[0K", 4);
}

/* Move to column "hpos", where hpos starts at 0 */
static void move_cursor_abs(int hpos)
{
    char buf[32];

    sprintf(buf, "\r\033[%dC", hpos);
    write(termfd, buf, strlen(buf));
}

static void send_input_buffer(int flags)
{
    char buf[1024];

    memcpy(buf, input_buf, input_len);

    send_input(buf, input_len, flags);
    input_len = input_pos = 0;
    reading = 0;
    echo = 1;
}

/* Input from keyboard */
int tty_process_terminal(unsigned char *buf, int len)
{
    int i;

    for (i=0; i<len; i++)
    {
	if (buf[i] == exit_char)
	{
	    finished = 1;
	    return 0;
	}

	/* Terminators */
	// TODO: flag for echoing terminators
	if (is_terminator(buf[i]))
	{
	    send_input(&buf[i], 1, 1); /* last "1" is "terminator" */
	    reading = 0;
	    return 0;
	}

	/* Swap LF for CR */
	//PJC: is this right??
	if (buf[i] == '\n')
	    buf[i] = '\r';


	/* Is it ESCAPE ? */
	if (buf[i] == ESC && esc_len == 0)
	{
	    esc_buf[esc_len++] = buf[i];
	    continue;
	}

	/* Still processing escape sequence */
	if (esc_len)
	{
	    esc_buf[esc_len++] = buf[i];
	    if (isalpha(buf[i]))
	    {
		/* Process escape sequences */
		if (strncmp(esc_buf, "\033[D", 3)) /* Cursor RIGHT */
		{
		    if (input_pos < input_len)
		    {
			input_pos++;
			if (echo) write(termfd, esc_buf, esc_len);
		    }
		}
		if (strncmp(esc_buf, "\033[C", 3)) /* Cursor LEFT */
		{
		    if (input_pos > 0)
		    {
			input_pos--;
			if (echo) write(termfd, esc_buf, esc_len);
		    }
		}
		esc_len = 0;
	    }
	    continue;
	}

	/* Process non-terminator control chars */
	if (buf[i] < ' ' || buf[i] == DEL)
	{
	    switch (buf[i])
	    {
	    case CTRL_B: // Up a line
		break;
	    case CTRL_X: // delete input line
	    case CTRL_U: // delete input line
		move_cursor_abs(prompt_len);
		if (echo) erase_eol();
		input_pos = input_len = 0;
		break;
	    case CTRL_H: // back to start of line
		if (echo) move_cursor_abs(prompt_len);
		input_pos = 0;
		break;
	    case CTRL_A: /* switch overstrike/insert mode */
		insert_mode = 1-insert_mode;
		break;
	    case CTRL_M:
		input_buf[input_len++] = buf[i];
		send_input_buffer(1);
		input_len = input_pos = 0;
		reading = 0;
		break;
	    case DEL:
		if (input_pos > 0)
		{
		    input_pos--;
		    input_len = input_pos;
		     if (echo) write(termfd, "\033[D \033[D", 7);
		}
		break;
	    }
	    continue;
	}

	/* Read not active, store in the read ahead buffer */
	if (!reading)
	{
	    if (rahead_len < sizeof(rahead_buf))
		rahead_buf[rahead_len++] = buf[i];
	    continue;
	}

	/* echo if req'd */
	if (echo)
	    write(termfd, &buf[i], 1);

	/* Check for buffer overflow */
	input_buf[input_pos++] = buf[i];
	if (input_len < input_pos)
	    input_len = input_pos;

	if (input_len >= max_read_len)
	{
	    send_input_buffer(4);
	}
    }
    return 0;
}
