/******************************************************************************
    (c) 2002-2005      P.J. Caulfield          patrick@debian.org

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
extern unsigned char char_attr[];

/* Input state buffers & variables */
static char terminators[32];
static char rahead_buf[128];
static int  rahead_len=0;
static char input_buf[1024];
static int  input_len=0;
static int  input_pos=0;
static char prompt_buf[1024];
static char prompt_len=0;
static char line_start_pos=0;
static char esc_buf[132];
static int  esc_len=0;
static int  max_read_len = sizeof(input_buf);
static int  echo = 1;
static int  reading = 0;
static int  insert_mode = 0;
static int  echo_terminator = 0;
static int  discard = 0;
static int  interpret_escape = 0;
static int  allow_edit = 0;
static int  convert_uppercase = 0;
static char last_char;

/* in dnlogin.c */
extern int  char_timeout;
extern int  timeout_valid;

/* Output processors */
int  (*send_input)(char *buf, int len, int term_pos, int flags);
int  (*send_oob)(char oobchar, int discard);
void (*rahead_change)(int count);

static void send_input_buffer(int flags)
{
	char buf[1024];

	memcpy(buf, input_buf, input_len);

	// Assumes the terminator (CR) is the last char...
	send_input(buf, input_len, input_len-1, flags);
	input_len = input_pos = 0;
	reading = 0;
	echo = 1;
	interpret_escape = 0;
}

/* Called when select() times out */
void tty_timeout()
{
	if (debug & 4)
		fprintf(stderr, "TTY: timeout\n");
	send_input_buffer(SEND_FLAG_TIMEOUT);
}

/* Raw write to terminal */
int tty_write(char *buf, int len)
{
	if (discard)
		return len;

	if (len == 1 && *buf == 0)
		return len;
	write(termfd, buf, len);
	last_char = buf[len-1];
	return len;
}

int tty_discard()
{
	return discard;
}

void tty_format_cr()
{
	char lf = '\n';

	if (debug & 4)
		fprintf(stderr, "TTY: format_cr, last char was %x\n", last_char);
	if (last_char == '\r')
		write(termfd, &lf, 1);
}

void tty_set_noecho()
{
	echo = 0;
}

void tty_set_discard(int onoff)
{
	discard = onoff;
}

void tty_set_uppercase(int onoff)
{
	convert_uppercase = onoff;
}

void tty_allow_edit(int onoff)
{
	allow_edit = onoff;
}

int tty_set_escape_proc(int onoff)
{
	int oldval = interpret_escape;
	interpret_escape = onoff;

	return interpret_escape;
}

int  tty_get_input_count(void)
{
	return input_len+rahead_len;
}

void tty_echo_terminator(int a)
{
	if (debug & 4)
		fprintf(stderr, "TTY: echo terminators = %d\n", a);

	echo_terminator = a;
}

void tty_set_default_terminators()
{
	if (debug & 4)
		fprintf(stderr, "TTY: set default terminators\n");

	/* All control chars except ^R ^U ^W, BS & HT */
	/* ie 18, 21, 23, 8, 9 */
	/* PJC: also remove ^A(1) ^E(5) ^X(29) for line-editting.. CHECK!! */
	memset(terminators, 0, sizeof(terminators));

	terminators[0] = 0xDD;
	terminators[1] = 0xFC;
	terminators[2] = 0x5B;
	terminators[3] = 0xDF;
}

void tty_set_terminators(char *buf, int len)
{
	if (debug & 4) {
		int i;
		fprintf(stderr, "TTY: set terminators... %d bytes\n", len);
		fprintf(stderr, "TTY: terms: ");
		for (i=0; i<len; i++)
			fprintf(stderr, "%02x ", buf[i]);
		fprintf(stderr, "\n");
	}
	memset(terminators, 0, sizeof(terminators));
	memcpy(terminators, buf, len);
}

void tty_start_read(char *prompt, int len, int promptlen)
{
	int i;
	if (debug & 4)
		fprintf(stderr, "TTY: start_read promptlen = %d, maxlen=%d\n",
			promptlen, len);
	if (promptlen) write(termfd, prompt, promptlen);
	if (len < 0) len = sizeof(input_buf);

	/* Save the actual prompt in one buffer and the prefilled
	   data in the input buffer */
	memcpy(prompt_buf, prompt, promptlen);
	prompt_len = promptlen;

	/* Work out the position of the cursor after the prompt (which may contain CRLF chars ) */
	line_start_pos = 0;
	for (i = 0; i<prompt_len; i++)
		if (prompt_buf[i] >= ' ')
			line_start_pos++;

	/* Prefilled data (eg a recalled command) */
	memcpy(input_buf, prompt+promptlen, len-promptlen);
	input_len = len-promptlen;
	input_pos = input_len;
	write(termfd, input_buf, input_len);

	max_read_len = len;

	/* Now add in any typeahead */
	if (rahead_len)
	{
		int copylen = rahead_len;

		if (debug & 4)
			fprintf(stderr, "TTY: readahead = %d bytes\n", rahead_len);

		/* Don't overflow the input buffer */
		if (input_len + copylen > sizeof(input_buf))
			copylen = sizeof(input_buf)-input_len;

		memcpy(input_buf+input_len, rahead_buf, copylen);
		tty_write(rahead_buf, copylen);
		input_len += copylen;
		input_pos += copylen;
		rahead_len = 0;
	}
	reading = 1;
}

void tty_set_timeout(unsigned short to)
{
	timeout_valid = 1;
	char_timeout = to;
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
		new_term.c_iflag &= ~INLCR;
		new_term.c_iflag |= IGNBRK;
		new_term.c_oflag &= ~ONLCR;
		new_term.c_cc[VMIN] = 1;
		new_term.c_cc[VTIME] = 0;
		new_term.c_lflag &= ~ICANON;
		new_term.c_lflag &= ~ISIG;
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
	aux = c % 8;
	msk = (1 << aux);

	if (debug & 4)
		fprintf(stderr, "TTY: is_terminator: %d: (byte=%x, msk=%x) %s\n", c, terminators[termind],msk,
			(terminators[termind] & msk)?"Yes":"No");

	if (terminators[termind] & msk)
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


void tty_send_unread()
{
	send_input_buffer(SEND_FLAG_UNREAD);
}


static void redraw_input_line(void)
{
	if (!echo)
		return;
	move_cursor_abs(line_start_pos);
	erase_eol();
	write(termfd, input_buf, input_len);
	move_cursor_abs(line_start_pos + input_pos);
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

		/* Swap LF for CR */
		//PJC: is this right??
		if (buf[i] == '\n')
			buf[i] = '\r';

		/* Check for OOB */
		if (char_attr[(int)buf[i]] & 3)
		{
			int oob_discard = (char_attr[(int)buf[i]]>>2)&1;

			send_oob(buf[i], oob_discard);
			if (oob_discard)
			{
				input_len = input_pos = 0;
			}
			continue;
		}

		/* Is it ESCAPE ? */
		if (buf[i] == ESC && esc_len == 0 && interpret_escape)
		{
			esc_buf[esc_len++] = buf[i];
			continue;
		}

		/* Terminators */
		if (!esc_len && is_terminator(buf[i]))
		{
			if (echo_terminator)
			{
				tty_write(&buf[i], 1);
			}
			input_buf[input_len++] = buf[i];
			send_input_buffer(SEND_FLAG_VALID_ESCAPE);
			reading = 0;
			continue;
		}

		/* Still processing escape sequence */
		if (esc_len)
		{
			esc_buf[esc_len++] = buf[i];
			if (isalpha(buf[i]) || buf[i] == '~')
			{
				int esc_done = 0;

				if (allow_edit)
				{
					/* Process escape sequences */
					if (strncmp(esc_buf, "\033[C", 3) == 0) /* Cursor RIGHT */
					{
						if (input_pos < input_len)
						{
							input_pos++;
							if (echo) tty_write(esc_buf, esc_len);
						}
						esc_done = 1;
					}
					if (strncmp(esc_buf, "\033[D", 3) == 0) /* Cursor LEFT */
					{
						if (input_pos > 0)
						{
							input_pos--;
							if (echo) tty_write(esc_buf, esc_len);
						}
						esc_done = 1;
					}
				}

				/* If we didn't process it above, then just send it to
				   the host on the end of whatever is also in the buffer */
				if (!esc_done)
				{
					memcpy(input_buf+input_len, esc_buf, esc_len);
					send_input(input_buf, input_len+esc_len, input_len, SEND_FLAG_VALID_ESCAPE);
					echo = 1;
				}
				esc_len = 0;
			}
			continue;
		}

		/* Process non-terminator control chars */
		if ((buf[i] < ' ' || buf[i] == DEL) && allow_edit)
		{
			switch (buf[i])
			{
			case CTRL_B: // Up a line
				break;
			case CTRL_X: // delete input line
			case CTRL_U: // delete input line
				move_cursor_abs(line_start_pos);
				if (echo) erase_eol();
				input_pos = input_len = 0;
				break;
			case CTRL_H: // back to start of line
				if (echo) move_cursor_abs(line_start_pos);
				input_pos = 0;
				break;
			case CTRL_E: // Move to end of line
				if (echo) move_cursor_abs(input_len+line_start_pos);
				input_pos = input_len;
				break;
			case CTRL_A: /* switch overstrike/insert mode */
				insert_mode = ~insert_mode;
				break;
			case CTRL_W: /* redraw */
				redraw_input_line();
				break;
			case CTRL_M:
				input_buf[input_len++] = buf[i];
				send_input_buffer(SEND_FLAG_TERMINATOR);
				input_len = input_pos = 0;
				reading = 0;
			case CTRL_O:
				discard = ~discard;
				break;
			case DEL:
				if (input_pos > 0)
				{
					input_pos--;
					input_len--;
					if (echo) write(termfd, "\033[D \033[D", 7);
					if (input_pos != input_len)
					{
						memmove(input_buf+input_pos, input_buf+input_pos+1, input_len-input_pos);
						redraw_input_line();
					}
					break;
				}
			}
			continue;
		}

		/* Read not active, store in the read ahead buffer */
		if (!reading)
		{
			if (rahead_len == 0)
				rahead_change(1); /* tell CTERM */

			if (rahead_len < sizeof(rahead_buf))
				rahead_buf[rahead_len++] = buf[i];
			continue;
		}

		if (convert_uppercase)
			buf[i] &= 0x5F;

		/* echo if req'd, insert will redraw the whole line */
		if (echo && !insert_mode)
			tty_write(&buf[i], 1);

		if (insert_mode && input_len > input_pos)
		{
			memmove(input_buf+input_pos+1, input_buf+input_pos, input_len-input_pos);
			input_len++;
		}
		input_buf[input_pos++] = buf[i];

		if (input_len < input_pos)
			input_len = input_pos;

		if (insert_mode)
			redraw_input_line();

		if (input_len >= max_read_len)
		{
			send_input_buffer(SEND_FLAG_BUFFER_FULL);
		}
	}
	return 0;
}
