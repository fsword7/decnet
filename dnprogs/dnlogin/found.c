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

static int sockfd = -1;

static int send_bind(void)
{
    int wrote;
    unsigned char bindmsg[] =
    {
// TODO annotate correctly
	4,
	2,4,0,
	7,0,
	16,0,
    };

    wrote = write(sockfd, bindmsg, sizeof(bindmsg));
    if (wrote != sizeof(bindmsg))
    {
	fprintf(stderr, "%s\n", found_connerror("read error"));
	return -1;
    }
    return 0;
}


int found_getsockfd()
{
    return sockfd;
}

int found_write(char *buf, int len)
{
    // TODO: write header then message with EOR using sendmsg
    return -1;
}

int found_read()
{
    int len;
    char inbuf[1024];

    if ( (len=dnet_recv(sockfd, inbuf, sizeof(inbuf), MSG_EOR|MSG_DONTWAIT))
	 <= 0)

    {
	fprintf(stderr, "%s\n", found_connerror("read sock"));
	return -1;
    }

    /* TODO: something about the foundation message codes */

    /* TODO: Know the protocol, switch cterm/dterm etc? */
    return process_cterm(inbuf+4, len-4);
}

/* Open the DECnet connection */
int found_setup_link(char *node, int object)
{
    struct nodeent *np;
    struct sockaddr_dn sockaddr;

    if ( (np=getnodebyname(node)) == NULL)
    {
	printf("Unknown node name %s\n",node);
	return -1;
    }


    if ((sockfd = socket(AF_DECnet, SOCK_SEQPACKET, DNPROTO_NSP)) == -1)
    {
	perror("socket");
	return -1;
    }

    sockaddr.sdn_family = AF_DECnet;
    sockaddr.sdn_flags = 0x00;
    sockaddr.sdn_objnum = object;

    sockaddr.sdn_objnamel = 0x00;
    sockaddr.sdn_add.a_len = 0x02;

    memcpy(sockaddr.sdn_add.a_addr, np->n_addr, 2);

    if (connect(sockfd, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0)
    {
	perror("socket");
	return -1;
    }

    return send_bind();
}


/* Return the text of a connection error */
char *found_connerror(char *default_msg)
{
#ifdef DSO_DISDATA
    struct optdata_dn optdata;
    unsigned int len = sizeof(optdata);
    char *msg;

    if (getsockopt(sockfd, DNPROTO_NSP, DSO_DISDATA,
		   &optdata, &len) == -1)
    {
	return default_msg;
    }

    // Turn the rejection reason into text
    switch (optdata.opt_status)
    {
    case DNSTAT_REJECTED: msg="Rejected by object";
	break;
    case DNSTAT_RESOURCES: msg="No resources available";
	break;
    case DNSTAT_NODENAME: msg="Unrecognised node name";
	break;
    case DNSTAT_LOCNODESHUT: msg="Local Node is shut down";
	break;
    case DNSTAT_OBJECT: msg="Unrecognised object";
	break;
    case DNSTAT_OBJNAMEFORMAT: msg="Invalid object name format";
	break;
    case DNSTAT_TOOBUSY: msg="Object too busy";
	break;
    case DNSTAT_NODENAMEFORMAT: msg="Invalid node name format";
	break;
    case DNSTAT_REMNODESHUT: msg="Remote Node is shut down";
	break;
    case DNSTAT_ACCCONTROL: msg="Login information invalid at remote node";
	break;
    case DNSTAT_NORESPONSE: msg="No response from object";
	break;
    case DNSTAT_NODEUNREACH: msg="Node Unreachable";
	break;
    case DNSTAT_MANAGEMENT: msg="Abort by management/third party";
	break;
    case DNSTAT_ABORTOBJECT: msg="Remote object aborted the link";
	break;
    case DNSTAT_NODERESOURCES: msg="Node does not have sufficient resources for a new link";
	break;
    case DNSTAT_OBJRESOURCES: msg="Object does not have sufficient resources for a new link";
	break;
    case DNSTAT_BADACCOUNT: msg="The Account field in unacceptable";
	break;
    case DNSTAT_TOOLONG: msg="A field in the access control message was too long";
	break;
    default: msg=default_msg;
	break;
    }
    return msg;
#else
    return strerror(errno);
#endif
}
