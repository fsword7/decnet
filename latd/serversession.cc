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

#include <sys/types.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <list>
#include <queue>
#include <string>
#include <map>
#include <strstream>

#include "lat.h"
#include "utils.h"
#include "session.h"
#include "connection.h"
#include "latcpcircuit.h"
#include "server.h"
#include "serversession.h"

ServerSession::ServerSession(class LATConnection &p, LAT_SessionStartCmd *cmd,
			     unsigned char remid, 
			     unsigned char localid, bool clean):
  LATSession(p, remid, localid, clean)
{
    max_read_size = cmd->dataslotsize;
    
    debuglog(("new server session: localid %d, remote id %d, data slot size: %d\n",
	    localid, remid, max_read_size));


}

int ServerSession::new_session(unsigned char *_remote_node, unsigned char c)
{
    credit = c;
    int status = create_session(_remote_node);
    if (status == 0)
    {
	status = send_login_response();
	if (credit) send_issue();
    }
    strcpy(remote_node, (char *)_remote_node);
    return status;
}


// Send the login response packet.
int ServerSession::send_login_response()
{
    unsigned char buf[1600];
    unsigned char slotbuf[256];
    LAT_SessionReply *reply = (LAT_SessionReply *)buf;
    int ptr = 0;
    int slotptr = 0;

    // ACK
    reply->header.cmd       = LAT_CCMD_SREPLY;
    reply->header.num_slots = 0;
    reply->slot.length      = 0;
    reply->slot.cmd         = 0x0;
    reply->slot.local_session = 0;
    reply->slot.remote_session = 0;
    parent.send_message(buf,sizeof(LAT_Header),LATConnection::REPLY);

    
    slotbuf[slotptr++] = 0x01; // Service Class
    slotbuf[slotptr++] = 0x01; // Min Attention slot size
    slotbuf[slotptr++] = max_read_size;
    
    slotbuf[slotptr++] = 0x00; // Dest service length/name
    slotbuf[slotptr++] = 0x00; // Source service length/name

    slotbuf[slotptr++] = 0x01; // Param type 1
    slotbuf[slotptr++] = 0x02; // Param Length 2
    slotbuf[slotptr++] = 0x00; // Value 0 (woz 1024: 04 00)
    slotbuf[slotptr++] = 0x00; // 

    slotbuf[slotptr++] = 0x04; // Param type 4 (PTY name)
    add_string(slotbuf, &slotptr, (unsigned char *)ptyname);
    slotbuf[slotptr++] = 0x00; // NUL terminated (??)

    add_slot(buf, ptr, 0x9f, slotbuf, slotptr);
    slotptr = 0;

    // Send a data_b slot
    slotbuf[slotptr++] = 0x26; // Flags
    slotbuf[slotptr++] = 0x13; // Stop  output char XOFF
    slotbuf[slotptr++] = 0x11; // Start output char XON
    slotbuf[slotptr++] = 0x13; // Stop  input char  XOFF
    slotbuf[slotptr++] = 0x11; // Start input char  XON

    // data_b slots count against credit
    if (credit)
    {
	add_slot(buf, ptr, 0xaf, slotbuf, slotptr);
	credit--;
    }
    slotptr = 0;

    // Stuff the client full of credits
    add_slot(buf, ptr, 0x0f, slotbuf, 0);
    add_slot(buf, ptr, 0x0f, slotbuf, 0); 

    slotbuf[slotptr++] = 0x0D;
    slotbuf[slotptr++] = 0x0A;
    slotbuf[slotptr++] = 0x0D;
    add_slot(buf, ptr, 0x0f, slotbuf, 0);

    parent.queue_message(buf, ptr);

    return 0;
}



int ServerSession::create_session(unsigned char *remote_node)
{
    int slave_fd;

    debuglog(("create session: local: %d, remote: %d\n",
	    local_session, remote_session));

    if (openpty(&master_fd,
		&slave_fd, NULL, NULL, NULL) != 0)
	return -1; /* REJECT */
  
    strcpy(ptyname, ttyname(slave_fd));
    state = STARTING;
    
    switch (fork())
    {
    case 0: // Child
    {
	int fd = slave_fd;

	// Set terminal characteristics
	struct termios tio;
	tcgetattr(slave_fd, &tio);
	tio.c_oflag |= ONLCR;
	tcsetattr(slave_fd, TCSANOW, &tio);
	
	close(master_fd);
	if (fd != 0) dup2(fd, 0);
	if (fd != 1) dup2(fd, 1);
	if (fd != 2) dup2(fd, 2);
	if (fd > 2) close (fd);
	
	setsid();

	// Older login programs don't take the -h flag
#ifdef SET_LOGIN_HOST
	execlp("/bin/login", "login", "-h", remote_node, (char *)0);
#else
	execlp("/bin/login", "login", (char *)0);
#endif
	// Argh!
	syslog(LOG_ERR, "Error in starting /bin/login: %m");

	// Exit now so that the parent will get EOF on the channel
	exit(-1);
    }
      
    case -1: // Failed
	syslog(LOG_ERR, "Error forking for /bin/login: %m");
      	perror("fork");
	close(master_fd);
	close(slave_fd);
	return -1;
      
    default: // Parent
	close(slave_fd);
	fcntl(master_fd, F_SETFL, fcntl(master_fd, F_GETFL, 0) | O_NONBLOCK);
	connected = true;
	sleep(0); // Give login a chance to start
	break;
    }
    return 0;
}
