/******************************************************************************
    (c) 2001-2002 Patrick Caulfield                 patrick@debian.org

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
#include "lloginsession.h"
#include "lat_messages.h"
#include "dn_endian.h"

lloginSession::lloginSession(class LATConnection &p,
			     unsigned char remid, unsigned char localid,
			     char *lta, int fd):
    ClientSession(p, remid, localid, lta, clean),
    have_been_queued(false)
{
    master_fd = dup(fd);
    // This is the socket FD - we dup it because the original
    // will be closed when it "changes" from a LLOGIN_FD to a PTY
    debuglog(("new llogin session: localid %d, remote id %d\n",
	    localid, remid));
    remote_node[0] = '\0';
}

int lloginSession::new_session(unsigned char *_remote_node,
			       char *service, char *port, char *password,
			       unsigned char c)
{
    credit = c;
    strcpy(remote_service, service);
    strcpy(remote_port, port);
    strcpy(remote_pass, password);

    debuglog(("lloginSession::new_session\n"));

    // Make it non-blocking so we can poll it
    fcntl(master_fd, F_SETFL, fcntl(master_fd, F_GETFL, 0) | O_NONBLOCK);

    // Auto-connect
    if (!connect_parent())
    {
	state = STARTING;

	// Disable reads on the PTY until we are connected (or it fails)
	LATServer::Instance()->set_fd_state(master_fd, true);
    }
    else
    {
	// Service does not exist or we haven't heard of it yet.
	disconnect_sock();
	return -1;
    }

    return 0;
}

void lloginSession::connect()
{
    debuglog(("connecting llogin session to '%s'\n", remote_service));
    state = RUNNING;

    // OK, now send a Start message to the remote end.
    unsigned char buf[1600];
    memset(buf, 0, sizeof(buf));
    LAT_SessionData *reply = (LAT_SessionData *)buf;
    int ptr = sizeof(LAT_SessionData);

    buf[ptr++] = 0x01; // Service Class
    buf[ptr++] = 0x01; // Max Attention slot size
    buf[ptr++] = 0xfe; // Max Data slot size

    add_string(buf, &ptr, (unsigned char *)remote_service);
    buf[ptr++] = 0x00; // Source service length/name

    buf[ptr++] = 0x01; // Param type 1
    buf[ptr++] = 0x02; // Param Length 2
    buf[ptr++] = 0x04; // Value 1024
    buf[ptr++] = 0x00; //

    buf[ptr++] = 0x05; // Param type 5 (Local PTY name)
    add_string(buf, &ptr, (unsigned char*)ltaname);

    // Add password if present.
    if (remote_pass[0] != '\0')
    {
	buf[ptr++] = 0x07; // Param type 7 (Service password)
	add_string(buf, &ptr, (unsigned char*)remote_pass);
    }

    // If the user wanted a particular port number then add it
    // into the message
    if (remote_port[0] != '\0')
    {
	debuglog(("Adding port %s\n", remote_port));
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

// Disconnect the local socket
void lloginSession::disconnect_sock()
{
    LATServer::Instance()->set_fd_state(master_fd, false);
    LATServer::Instance()->delete_session(parent.get_connection_id(), local_session, master_fd);
}


// Remote end disconnects or EOF on local PTY
void lloginSession::disconnect_session(int reason)
{
    debuglog(("lloginSession::disconnect_session()\n"));
    // If the reason was some sort of error then send it to
    // the user.
    if (reason > 1)
    {
	char *msg = lat_messages::session_disconnect_msg(reason);
	write(master_fd, msg, strlen(msg));
	write(master_fd, "\n", 1);
    }
    connected = false;
    disconnect_sock();
    return;
}


lloginSession::~lloginSession()
{
    close (master_fd);
    LATServer::Instance()->remove_fd(master_fd);
}

void lloginSession::do_read()
{
    read_pty();
}

void lloginSession::show_status(unsigned char *node, LAT_StatusEntry *entry)
{
    char buffer[1024];
    int len;

    if (entry->max_que_pos != 0)
    {
	len = snprintf(buffer, sizeof(buffer), "LAT: You are queued for %s, position %d\n",
		       node, dn_ntohs(entry->max_que_pos));

	write(master_fd, buffer, len);
	have_been_queued = true;
    }
}


// Called when a PortSession connects to us
void lloginSession::start_port()
{
    if (have_been_queued)
	write(master_fd, "LAT: Connected\n", 15);
}

