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
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <list>
#include <queue>
#include <string>
#include <map>

#include "lat.h"
#include "utils.h"
#include "session.h"
#include "connection.h"
#include "latcpcircuit.h"
#include "server.h"
#include "clientsession.h"

ClientSession::ClientSession(class LATConnection &p, 
			     unsigned char remid, unsigned char localid,
			     char *ttyname):
  LATSession(p, remid, localid)
{
    debuglog(("new client session: localid %d, remote id %d\n",
	    localid, remid));
    if (ttyname) strcpy(ltaname, ttyname);
}

int ClientSession::new_session(unsigned char *remote_node, unsigned char c)
{
    credit = c;
    if (openpty(&master_fd,
		&slave_fd, NULL, NULL, NULL) != 0)
	return -1; /* REJECT */
  
    strcpy(ptyname, ttyname(slave_fd));
    strcpy(mastername, ttyname(master_fd));
    state = STARTING;
    
    // Check for /dev/lat & create it if necessary
    struct stat st;
    if (stat("/dev/lat", &st) == -1)
    {
	mkdir("/dev/lat", 0755);
    }

    // Link the PTY to the actual LTA name we were passed
    if (ltaname[0] == '\0')
    {
	sprintf(ltaname, "/dev/lat/lta%d", local_session);
    }
    unlink(ltaname);
    symlink(ptyname, ltaname);

    fcntl(master_fd, F_SETFL, fcntl(master_fd, F_GETFL, 0) | O_NONBLOCK);

    debuglog(("made symlink %s to %s\n", ltaname, ptyname));
    LATServer::Instance()->add_pty(this, master_fd);
    return 0;
}

void ClientSession::connect_parent()
{
    parent.connect();
}

void ClientSession::connect()
{
    debuglog(("connecting client session\n"));

    // OK, now send a Start message to the remote end.
    unsigned char buf[1600];
    LAT_SessionData *reply = (LAT_SessionData *)buf;
    int ptr = sizeof(LAT_SessionData);
    
    buf[ptr++] = 0x01; // Service Class
    buf[ptr++] = 0x01; // Min Attention slot size
    buf[ptr++] = 0xfe; // Min Data slot size
    
    add_string(buf, &ptr, (unsigned char *)remote_node); 
    buf[ptr++] = 0x00; // Source service length/name

    buf[ptr++] = 0x01; // Param type 1
    buf[ptr++] = 0x02; // Param Length 2
    buf[ptr++] = 0x00; // Value 1024
    buf[ptr++] = 0x00; // 

    buf[ptr++] = 0x04; // Param type 4 (PTY name)
    add_string(buf, &ptr, (unsigned char *)ltaname);
    buf[ptr++] = 0x00; // NUL terminated (??)
 
    // Send message...
    reply->header.cmd          = LAT_CCMD_SDATA;
    reply->header.num_slots    = 1;
    reply->slot.remote_session = local_session;
    reply->slot.local_session  = remote_session;
    reply->slot.length         = ptr - sizeof(LAT_SessionData);
    reply->slot.cmd            = 0x9f;

    parent.send_message(buf, ptr, LATConnection::DATA);

}

ClientSession::~ClientSession()
{
    unlink(ltaname);
    
    close (slave_fd);
    close (master_fd);
    LATServer::Instance()->remove_fd(master_fd);
}

void ClientSession::do_read()
{
    debuglog(("ClientSession::do_read()\n"));
    if (!connected)
    {
	connect_parent();
	state = STARTING;

	// Disable reads on the PTY until we are connected (or it failed)
	LATServer::Instance()->set_fd_state(master_fd, true);
    }

    if (connected)
    {
	read_pty();
    }
}

void ClientSession::got_connection(unsigned char _remid)
{
    debuglog(("ClientSession:: got connection for rem session %d\n", _remid));
    LATServer::Instance()->set_fd_state(master_fd, false);
    remote_session = _remid;
    connected = true;
}
