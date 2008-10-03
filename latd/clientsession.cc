/******************************************************************************
    (c) 2001-2008 Christine Caulfield                 christine.caulfield@googlemail.com

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
#include <string.h>
#include <list>
#include <cassert>
#include <queue>
#include <string>
#include <map>
#include <sstream>

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
    return 0;
}

int ClientSession::connect_parent()
{
    debuglog(("connecting parent for %s\n", ltaname));
    return parent.connect(this);
}

// Disconnect the local PTY
void ClientSession::restart_pty()
{
    assert(!"ClientSession::restart_pty()\n");
}


// Remote end disconnects or EOF on local PTY
void ClientSession::disconnect_session(int reason)
{
    debuglog(("ClientSession::disconnect_session()\n"));
    // If the reason was some sort of error then send it to
    // the PTY
    if (reason > 1)
    {
	const char *msg = lat_messages::session_disconnect_msg(reason);
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

// Called from the slave connection - return the master fd so it can
// can do I/O on it.
int ClientSession::get_port_fd()
{
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
