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

// We get sent (in order)
// 11, 7, 2, 7
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
static int cterm_process_initiate(char *buf, int len)
{
    unsigned char initsq[] =
	{ 0x01, 0x00, 0x01, 0x04, 0x00,
	  'd', 'n', 'l', 'o', 'g', 'i', 'n', ' ',
	  0x01, 0x02, 0x00, 0x02,	/* Max msg size */
	  0x02, 0x02, 0xF4, 0x03,	/* Max input buf */
	  0x03, 0x04, 0xFE, 0x7F, 0x00,	/* Supp. Msgs */
	  0x00};

    found_common_write(initsq, sizeof(initsq));
    return len;
}

static int cterm_process_start_read(char *buf, int len)
{
    unsigned short flags     = buf[1] | buf[2]<<8;
    unsigned short maxlength = buf[3] | buf[4]<<8;
    unsigned short eodata    = buf[5] | buf[6]<<8;
    unsigned short timeout   = buf[7] | buf[8]<<8;
    unsigned short eoprompt  = buf[9] | buf[10]<<8;
    unsigned short sodisplay = buf[11]| buf[12]<<8;
    unsigned short lowwater  = buf[13]| buf[14]<<8;
    unsigned char  term_len  = buf[16];

// TODO flags

    tty_set_terminators(buf+17, term_len);
    tty_start_read(buf+17+term_len, len-term_len-16, eoprompt);
    tty_set_timeout(timeout);
    tty_set_maxlen(maxlength);

    return len;
}

static int cterm_process_read_data(char *buf, int len)
{return len;}

static int cterm_process_oob(char *buf, int len)
{return len;}

static int cterm_process_unread(char *buf, int len)
{return len;}

static int cterm_process_clear_input(char *buf, int len)
{return len;}

static int cterm_process_write(char *buf, int len)
{
    unsigned short flags = buf[1] | buf[2]<<8;
    unsigned char prefixdata = buf[3];
    unsigned char postfixdata = buf[4];

    // TODO: flags...
    tty_write(buf+5, len-5);
    return len;
}

static int cterm_process_write_complete(char *buf, int len)
{return len;}

static int cterm_process_discard_state(char *buf, int len)
{return len;}

static int cterm_process_read_characteristics(char *buf, int len)
{return len;}

static int cterm_process_characteristics(char *buf, int len)
{return len;}

static int cterm_process_check_input(char *buf, int len)
{return len;}

static int cterm_process_input_count(char *buf, int len)
{return len;}

static int cterm_process_input_state(char *buf, int len)
{return len;}

/* Process buffer from cterm host */
int process_cterm(char *buf, int len)
{
    int offset = 0;

    while (offset < len)
    {
	if (debug > 2) fprintf(stderr, "CTERM: got msg: %d, len=%d\n",
			       buf[offset], len);

	switch (buf[offset])
	{
	case CTERM_MSG_INITIATE:
	    offset += cterm_process_initiate(buf+offset, len-offset);
	    break;
	case CTERM_MSG_START_READ:
	    offset += cterm_process_start_read(buf+offset, len-offset);
	    break;
	case CTERM_MSG_READ_DATA:
	    offset += cterm_process_read_data(buf+offset, len-offset);
	    break;
	case CTERM_MSG_OOB:
	    offset += cterm_process_oob(buf+offset, len-offset);
	    break;
	case CTERM_MSG_UNREAD:
	    offset += cterm_process_unread(buf+offset, len-offset);
	    break;
	case CTERM_MSG_CLEAR_INPUT:
	    offset += cterm_process_clear_input(buf+offset, len-offset);
	    break;
	case CTERM_MSG_WRITE:
	    offset += cterm_process_write(buf+offset, len-offset);
	    break;
	case CTERM_MSG_WRITE_COMPLETE:
	    offset += cterm_process_write_complete(buf+offset, len-offset);
	    break;
	case CTERM_MSG_DISCARD_STATE:
	    offset += cterm_process_discard_state(buf+offset, len-offset);
	    break;
	case CTERM_MSG_READ_CHARACTERISTICS:
	    offset += cterm_process_read_characteristics(buf+offset, len-offset);
	    break;
	case CTERM_MSG_CHARACTERISTINCS:
	    offset += cterm_process_characteristics(buf+offset, len-offset);
	    break;
	case CTERM_MSG_CHECK_INPUT:
	    offset += cterm_process_check_input(buf+offset, len-offset);
	    break;
	case CTERM_MSG_INPUT_COUNT:
	    offset += cterm_process_input_count(buf+offset, len-offset);
	    break;
	case CTERM_MSG_INPUT_STATE:
	    offset += cterm_process_input_state(buf+offset, len-offset);
	    break;

	default:
	    fprintf(stderr, "Unknown cterm message %d received\n",
		    buf[offset]);
	    return -1;
	}
    }
    return 0;
}


int cterm_write(char *buf, int len)
{
    char newbuf[len+9];

    newbuf[0] =0;
    newbuf[1] =0;
    newbuf[2] =0;
    newbuf[3] =0;
    memcpy(newbuf+4, buf, len);

    return found_common_write(newbuf, len+4);
}
