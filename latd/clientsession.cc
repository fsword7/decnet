/******************************************************************************
    (c) 2001 Patrick Caulfield                 patrick@debian.org

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
#include <termios.h>
#include <list>
#include <queue>
#include <string>
#include <map>
#include <strstream>

#include "lat.h"
#include "utils.h"
#include "session.h"
#include "localport.h"
#include "connection.h"
#include "circuit.h"
#include "latcpcircuit.h"
#include "server.h"
#include "clientsession.h"
#include "lat_messages.h"

ClientSession::ClientSession(class LATConnection &p, 
			     unsigned char remid, unsigned char localid,
			     char *ttyname, bool clean):
  LATSession(p, remid, localid, clean),
  slave_fd_open(false)
{
    debuglog(("new client session: localid %d, remote id %d\n",
	    localid, remid));
    if (ttyname) strcpy(ltaname, ttyname);
}

// This should never be called now as it is overridden
// by all self-respecting superclasses.
int ClientSession::new_session(unsigned char *_remote_node, 
			       char *service, char *port,
			       unsigned char c)
{

    assert(!"Should never get here!!");


    credit = c;
    strcpy(remote_service, service);
    strcpy(remote_port, port);

// A quick word of explanation here.
// We keep the slave fd open after openpty because otherwise
// the master would go away too (EOF). When the user connects to
// the port we then close the slave fd so that we get EOF when she
// disconnects. got that?

    if (openpty(&master_fd,
		&slave_fd, NULL, NULL, NULL) != 0)
	return -1; /* REJECT */
  

    // Set terminal characteristics
    struct termios tio;
    tcgetattr(master_fd, &tio);
    tio.c_iflag |= IGNBRK|BRKINT;
    tcsetattr(master_fd, TCSANOW, &tio);

    strcpy(remote_node, (char *)_remote_node);
    strcpy(ptyname, ttyname(slave_fd));
    strcpy(mastername, ttyname(master_fd));
    state = NEW;
    slave_fd_open = true;
    
    // Check for /dev/lat & create it if necessary
    struct stat st;
    if (stat(LAT_DIRECTORY, &st) == -1)
    {
	mkdir(LAT_DIRECTORY, 0755);
    }

    // Link the PTY to the actual LTA name we were passed
    if (ltaname[0] == '\0')
    {
	sprintf(ltaname, LAT_DIRECTORY "lta%d", local_session);
    }
    unlink(ltaname);
    symlink(ptyname, ltaname);

    // Make it non-blocking so we can poll it
    fcntl(master_fd, F_SETFL, fcntl(master_fd, F_GETFL, 0) | O_NONBLOCK);

#ifdef USE_OPENPTY
    // Set it owned by "lat" if it exists. We only do this for
    // /dev/pts PTYs.
    gid_t lat_group = LATServer::Instance()->get_lat_group();
    if (lat_group)
    {
	chown(ptyname, 0, lat_group);
	chmod(ptyname, 0660);
    }
#endif

    debuglog(("made symlink %s to %s\n", ltaname, ptyname));
//    LATServer::Instance()->add_pty(this, master_fd);
    return 0;
}

int ClientSession::connect_parent()
{
    debuglog(("connecting parent for %s\n", ltaname));
    return parent.connect(this);
}

void ClientSession::connect()
{
    state = RUNNING;
    debuglog(("connecting client session to '%s'\n", remote_service));

    // OK, now send a Start message to the remote end.
    unsigned char buf[1600];
    memset(buf, 0, sizeof(buf));
    LAT_SessionData *reply = (LAT_SessionData *)buf;
    int ptr = sizeof(LAT_SessionData);
    
    buf[ptr++] = 0x01; // Service Class
    buf[ptr++] = 0x01; // Max Attention slot size..
    buf[ptr++] = 0xfe; // Max Data slot size
    
    add_string(buf, &ptr, (unsigned char *)remote_service);
    buf[ptr++] = 0x00; // Source service length/name

    buf[ptr++] = 0x01; // Param type 1
    buf[ptr++] = 0x02; // Param Length 2
    buf[ptr++] = 0x04; // Value 1024
    buf[ptr++] = 0x00; // 

    buf[ptr++] = 0x05; // Param type 5 (Local PTY name)
    add_string(buf, &ptr, (unsigned char *)ltaname);

    // If the user wanted a particular port number then add it 
    // into the message
    if (remote_port[0] != '\0')
    {
	buf[ptr++] = 0x04; // Param type 4 (Remote port name)
	add_string(buf, &ptr, (unsigned char *)remote_port);
	buf[ptr++] = 0x00; // NUL terminated (??)
    }
 
    // Send message...
    reply->header.cmd          = LAT_CCMD_SESSION;
    reply->header.num_slots    = 1;
    reply->slot.remote_session = local_session;
    reply->slot.local_session  = remote_session;
    reply->slot.length         = ptr - sizeof(LAT_SessionData);
    reply->slot.cmd            = 0x9f;

    parent.send_message(buf, ptr, LATConnection::DATA);

}

// Disconnect the local PTY
void ClientSession::restart_pty()
{
    assert(!"ClientSession::restart_pty()\n");
    connected = false;
    remote_session = 0;
    
    // Close it all down so the local side gets EOF
    unlink(ltaname);
    
    if (slave_fd_open) close (slave_fd);
    close (master_fd);
    LATServer::Instance()->set_fd_state(master_fd, true);
    LATServer::Instance()->remove_fd(master_fd);
    
    // Now open it all up again ready for a new connection
    new_session((unsigned char *)remote_node, remote_service, remote_port, 0);
}


// Remote end disconnects or EOF on local PTY
void ClientSession::disconnect_session(int reason)
{
    debuglog(("ClientSession::disconnect_session()\n"));
    // If the reason was some sort of error then send it to 
    // the PTY
    if (reason > 1)
    {
	char *msg = lat_messages::session_disconnect_msg(reason);
	write(master_fd, msg, strlen(msg));
	write(master_fd, "\n", 1);
    }
    LATServer::Instance()->set_fd_state(master_fd, true);
    connected = false;
    restart_pty();
    return;
}


ClientSession::~ClientSession()
{    
    if (slave_fd_open) close (slave_fd);
    if (master_fd > -1) 
    {
	close (master_fd);
	LATServer::Instance()->remove_fd(master_fd);
    }
}

void ClientSession::do_read()
{
    debuglog(("ClientSession::do_read(), connected: %d\n", connected));
    if (!connected)
    {
	if (!connect_parent())
	{
	    state = STARTING;
	    
	    // Disable reads on the PTY until we are connected (or it fails)
	    LATServer::Instance()->set_fd_state(master_fd, true);

	    close(slave_fd);
	    slave_fd_open = false;
	}
	else
	{
	    // Service does not exist or we haven't heard of it yet.
	    restart_pty();
	}
    }

    if (connected)
    {
	read_pty();
    }
}

void ClientSession::got_connection(unsigned char _remid)
{
    unsigned char buf[1600];
    LAT_SessionReply *reply = (LAT_SessionReply *)buf;
    int ptr = 0;

    debuglog(("ClientSession:: got connection for rem session %d\n", _remid));
    LATServer::Instance()->set_fd_state(master_fd, false);
    remote_session = _remid;

    // Send a data_b slot
    reply->header.cmd       = LAT_CCMD_SESSION;
    reply->header.num_slots = 0;
    reply->slot.length      = 0;
    reply->slot.cmd         = 0x0;
    reply->slot.local_session = 0;
    reply->slot.remote_session = 0;

    // data_b slots count against credit
    if (credit)
    {
	unsigned char slotbuf[256];
	int slotptr = 0;

	slotbuf[slotptr++] = 0x26; // Flags
	slotbuf[slotptr++] = 0x13; // Stop  output char XOFF
	slotbuf[slotptr++] = 0x11; // Start output char XON
	slotbuf[slotptr++] = 0x13; // Stop  input char  XOFF
	slotbuf[slotptr++] = 0x11; // Start input char  XON
	
	add_slot(buf, ptr, 0xaf, slotbuf, slotptr);
	credit--;
    }
    parent.queue_message(buf, ptr);

    connected = true;
}

// Called from the slave connection - return the master fd so it can 
// can do I/O on it and close the slave so it gets EOF notification.
int ClientSession::get_port_fd()
{
    close(slave_fd);
    return master_fd;
}

// Normal client sessions don't provide feedback on status (though maybe we 
// should check for other status types....
void ClientSession::show_status(unsigned char *node, LAT_StatusEntry *entry)
{
    return;
}

// Called when a PortSession connects to us
void ClientSession::start_port()
{
    return;
}
