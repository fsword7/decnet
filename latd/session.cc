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

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
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
#include <pwd.h>
#include <pty.h>
#include <assert.h>

#include <list>
#include <string>
#include <map>
#include <queue>
#include <strstream>

#include "lat.h"
#include "utils.h"
#include "session.h"
#include "connection.h"
#include "latcpcircuit.h"
#include "server.h"

void LATSession::add_credit(signed short c)
{
    if (credit < 0) 
    {
	debuglog(("ARGGHH: credit is negative\n"));
	credit = 0;
    }
    credit += c;
    
    if (stopped && credit)
    {
	debuglog(("Got some more credit, (+%d=%d) carrying on\n", c, credit));
	stopped = false;
    }
}

/* Got some data from the terminal - send it to the PTY */
int LATSession::send_data_to_process(unsigned char *buf, int len)
{  
    // If there's anything to send, do so
    if (len)
    {
#ifdef REALLY_VERBOSE_DEBUGLOG  
	char debugbuf[1024];
        memcpy(debugbuf, buf, len);
	debugbuf[len] = 0;
	debuglog(("To PTY(%d): %s\n", len, debugbuf));
#endif
	remote_credit--;

	// Replace LF/CR with LF if we are a server.
        // TODO There MUST be an option for this...
	if (!parent.isClient())
	{
	    char newbuf[len+1];
	    int newlen = 0;
	    int i;
	    for (i=0; i<len; i++)
	    {
		if (i>0 && i<len && (buf[i] == '\r') && (buf[i-1] == '\n')) 
		{
		    continue;
		}
		if (i>0 && i<len && (buf[i] == '\n') && (buf[i-1] == '\r'))
		{
		    newbuf[newlen-1] = '\n';
		    continue;
		}
		newbuf[newlen++] = buf[i];
	    }
	    write(master_fd, newbuf, newlen);
	}
	else
	{
	    write(master_fd, buf, len);
	}
    }

    // See if there's any echo. Anything longer than the data sent
    // is not a character echo and so will be sent as
    // unsolicited data.
    int numbytes;
    sleep(0); // Give it a slight chance of generating an echo
    ioctl(master_fd, FIONREAD, &numbytes);
    debuglog(("%d echo bytes available\n", numbytes));
    if (numbytes == 0 || numbytes != len)
    {
	echo_expected = false;
	return 1;
    }
    echo_expected = true;
    return 0;
}

/* Read some data from the PTY and send it to the LAT terminal */
int LATSession::read_pty()
{
    unsigned char buf[256];
    int  command = 0x00;
    int  msglen;

    if (credit <= 0) return 0; // Not allowed!
    
    msglen = read(master_fd, buf, max_read_size);

#ifdef REALLY_VERBOSE_DEBUGLOG  
    if (msglen > 0)
    {
      	buf[msglen] = '\0';
	char tmp = buf[10];
	buf[10] = '\0'; // Just a sample	    
	debuglog(("Session %d From PTY(%d): '%s%s'\n", local_session, msglen, 
		  buf, (msglen>10)?"...":""));
	buf[10] = tmp;
    }
#endif
	
    // EOF or error on PTY - tell remote end to disconnect
    if (msglen <= 0)
    {
	if (errno == EAGAIN) return 0; // Just no data.
	
	debuglog(("EOF on PTY\n"));
	unsigned char slotbuf[5];
	int ptr = 0;

	// Send attention & disconnect
	slotbuf[0] = 0x40;
	add_slot(buf, ptr, 0xb0, slotbuf, 1);

	// Tru64 (and DS500 I think) don't like the 
	// local session ID to be set on the final disconnect slot
	int s=local_session; 
        local_session=0;

	add_slot(buf, ptr, 0xd1, slotbuf, 0);
	local_session=s; // Restore it for our benefit.

	parent.queue_message(buf, ptr);
	
	disconnect_session(1); // User requested
	return 0;
    }

    // Got break!
    if (msglen == 1 && buf[0] == '\0')
	return send_break();
    else
	return send_data(buf, msglen, command);
}

int LATSession::send_break()
{
    unsigned char buf[1600];
    unsigned char slotbuf[10];
    int  ptr = 0;
    int command = 0xA0;

    syslog(LOG_INFO, "Sending BREAK\n");
    if (remote_credit <= 1)
    {
	debuglog(("Sending more remote credit with break\n"));
	remote_credit += 5;
	command |= 0x5;
    }

    slotbuf[0] = 0x10;
    add_slot(buf, ptr, command, slotbuf, 1);

    parent.queue_message(buf, ptr);
    return 0;
}

int LATSession::send_data(unsigned char *buf, int msglen, int command)
{
    unsigned char reply[1600];
    LAT_SlotCmd *header = (LAT_SlotCmd *)reply;
    int  ptr;

#ifdef REALLY_VERBOSE_DEBUG
    debuglog(("Local Credit stands at %d\n", credit));
    debuglog(("Remote Credit stands at %d\n", remote_credit));
#endif

    if (remote_credit <= 1)
    {
	debuglog(("Sending more remote credit\n"));
	remote_credit += 5;
	command |= 0x5;
    }
    
    // Send response...
    header->remote_session = local_session;
    header->local_session  = remote_session;
    header->length         = msglen;
    header->cmd            = command;

    ptr = sizeof(LAT_SlotCmd);
    memcpy(reply+ptr, buf, msglen);

    parent.send_slot_message(reply, ptr+msglen);
    
    // Have we now run out of credit ??
    if (--credit <= 0)
    {
	debuglog(("Out of credit...Stop\n"));
	stopped = true;
    }
    return 0;
}


// Remote end disconnects
void LATSession::disconnect_session(int reason)
{
    // Get the server to delete us when we are off the stack.
    if (connected)
	LATServer::Instance()->delete_session(&parent, local_session, master_fd);
    connected = false;
    return;
}


void LATSession::send_disabled_message()
{
    unsigned char replybuf[1600];
    LAT_SessionReply *reply = (LAT_SessionReply *)replybuf;

    debuglog(("Sending DISABLED message\n"));
    
    reply->header.cmd          = LAT_CCMD_SREPLY;
    reply->header.num_slots    = 1;
    reply->slot.remote_session = local_session;
    reply->slot.local_session  = remote_session;
    reply->slot.length         = 0;
    reply->slot.cmd            = 0xc8;  // Disconnect session.

    parent.send_message(replybuf, sizeof(LAT_SessionReply), 
			LATConnection::REPLY);
}

LATSession::~LATSession()
{
    debuglog(("Session dtor start\n"));
    if (pid != -1) kill(pid, SIGTERM);
    if (master_fd > -1) close(master_fd);
    disconnect_session(0);
}


// Send the issue.net greeting file
void LATSession::send_issue()
{
    // Send /etc/issue.net
    int f = open("/etc/issue.net", O_RDONLY);
    if (f >= 0)
    {
	unsigned char *issue = new unsigned char[255];
	unsigned char *newissue = new unsigned char[255];
	
	size_t len = read(f, issue, 255);
	close(f);
	
	size_t newlen = 0;	
	if (len > 255) len = 255;

	// Start with a new line
	newissue[newlen++] = '\r';
	newissue[newlen++] = '\n';

	// Add CR to all the LFs
	for (unsigned int i=0; i<len && newlen<255; )
	{
	    if (issue[i] == '\n')
		newissue[newlen++] = '\r';

	    newissue[newlen++] = issue[i++];
	}	
	
	echo_expected = false;

	send_data(newissue, newlen, 0x01);
	delete[] issue;
	delete[] newissue;
    }
}

void LATSession::set_port(unsigned char *inbuf)
{
    unsigned char *ptr = inbuf+sizeof(LAT_SlotCmd);

    if (*ptr & 0x10) /* BREAK */
    {
	debuglog(("Sending break\n"));
	tcsendbreak(master_fd, 0);
    }
}

// Add a slot to an existing message
void LATSession::add_slot(unsigned char *buf, int &ptr, int slotcmd, 
			  unsigned char *slotdata, int len)
{
    // Allow the caller to initialize ptr to zero and we will do the rest
    if (ptr == 0) 
    {
	ptr = sizeof(LAT_Header);
	LAT_Header *h = (LAT_Header *)buf;
	h->num_slots = 0;	
    }
    
    // Write the slot header
    LAT_SlotCmd *slot = (LAT_SlotCmd *)(buf+ptr);
    ptr += sizeof(LAT_SlotCmd);
    slot->length = len;
    slot->cmd    = slotcmd;
    slot->remote_session = local_session;
    slot->local_session  = remote_session;

    
    // Copy the data
    memcpy(buf+ptr, slotdata, len);
    ptr += len;
    if (ptr%2) ptr++;  // Word aligned

    
    // Increment the number of slots.
    LAT_Header *header = (LAT_Header *)buf;
    header->num_slots++;
    header->cmd = LAT_CCMD_SDATA;
}