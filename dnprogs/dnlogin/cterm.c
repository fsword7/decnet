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


#define CTERM_MSG_INITIATE               1
#define CTERM_MSG_START_READ             2
#define CTERM_MSG_READ_DATA              3
#define CTERM_MSG_OOB                    4
#define CTERM_MSG_UNREAD                 5
#define CTERM_MSG_CLEAR_INPUT            6
#define CTERM_MSG_WRITE                  7
#define CTERM_MSG_WRITE_COMPLETE         8
#define CTERM_MSG_DISCARD_STATE          9
#define CTERM_MSG_READ_CHARACTERISTICS  10
#define CTERM_MSG_CHARACTERISTINCS      11
#define CTERM_MSG_CHECK_INPUT           12
#define CTERM_MSG_INPUT_COUNT           13
#define CTERM_MSG_INPUT_STATE           14

/* Process incoming CTERM messages */
static int ct_process_initiate(char *buf, int len)
{
    return len;
}

static int ct_process_start_read(char *buf, int len)
{return len;}

static int ct_process_read_data(char *buf, int len)
{return len;}

static int ct_process_oob(char *buf, int len)
{return len;}

static int ct_process_unread(char *buf, int len)
{return len;}

static int ct_process_clear_input(char *buf, int len)
{return len;}

static int ct_process_write(char *buf, int len)
{return len;}

static int ct_process_write_complete(char *buf, int len)
{return len;}

static int ct_process_discard_state(char *buf, int len)
{return len;}

static int ct_process_read_characteristics(char *buf, int len)
{return len;}

static int ct_process_characteristics(char *buf, int len)
{return len;}

static int ct_process_check_input(char *buf, int len)
{return len;}

static int ct_process_input_count(char *buf, int len)
{return len;}

static int ct_process_input_state(char *buf, int len)
{return len;}

/* Process buffer from cterm host */
int process_cterm(char *buf, int len)
{
    int offset = 0;

    if (debug > 3)
    {
	int i;

	fprintf(stderr, "got message %d bytes:\n", len);
	for (i=0; i<len; i++)
	    fprintf(stderr, "%02x  ", buf[i]);
	fprintf(stderr, "\n\n");
    }

    while (offset < len)
    {
	if (debug > 2) fprintf(stderr, "dnlogin: got msg: %d, len=%d\n",
			       buf[offset], len);

	switch (buf[offset])
	{
	case CTERM_MSG_INITIATE:
	    offset += ct_process_initiate(buf+offset, len-offset);
	    break;
	case CTERM_MSG_START_READ:
	    offset += ct_process_start_read(buf+offset, len-offset);
	    break;
	case CTERM_MSG_READ_DATA:
	    offset += ct_process_read_data(buf+offset, len-offset);
	    break;
	case CTERM_MSG_OOB:
	    offset += ct_process_oob(buf+offset, len-offset);
	    break;
	case CTERM_MSG_UNREAD:
	    offset += ct_process_unread(buf+offset, len-offset);
	    break;
	case CTERM_MSG_CLEAR_INPUT:
	    offset += ct_process_clear_input(buf+offset, len-offset);
	    break;
	case CTERM_MSG_WRITE:
	    offset += ct_process_write(buf+offset, len-offset);
	    break;
	case CTERM_MSG_WRITE_COMPLETE:
	    offset += ct_process_write_complete(buf+offset, len-offset);
	    break;
	case CTERM_MSG_DISCARD_STATE:
	    offset += ct_process_discard_state(buf+offset, len-offset);
	    break;
	case CTERM_MSG_READ_CHARACTERISTICS:
	    offset += ct_process_read_characteristics(buf+offset, len-offset);
	    break;
	case CTERM_MSG_CHARACTERISTINCS:
	    offset += ct_process_characteristics(buf+offset, len-offset);
	    break;
	case CTERM_MSG_CHECK_INPUT:
	    offset += ct_process_check_input(buf+offset, len-offset);
	    break;
	case CTERM_MSG_INPUT_COUNT:
	    offset += ct_process_input_count(buf+offset, len-offset);
	    break;
	case CTERM_MSG_INPUT_STATE:
	    offset += ct_process_input_state(buf+offset, len-offset);
	    break;

	default:
	    fprintf(stderr, "Unknown cterm message %d received\n",
		    buf[offset]);
	    return -1;
	}
    }
    return 0;
}
