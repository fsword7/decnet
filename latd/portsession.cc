/******************************************************************************
    (c) 2000 Patrick Caulfield                 patrick@debian.org

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
#include <sys/signal.h>
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
#include "circuit.h"
#include "latcpcircuit.h"
#include "server.h"
#include "serversession.h"
#include "clientsession.h"
#include "portsession.h"

PortSession::PortSession(class LATConnection &p, LAT_SessionStartCmd *cmd,
			 ClientSession *_client,
			 unsigned char remid, unsigned char localid, 
			 bool clean):
    ServerSession(p, cmd, remid, localid, clean),
    client_session(_client)
{
    max_read_size = cmd->dataslotsize;
    master_fd = client_session->get_port_fd();
    
    debuglog(("new port session: localid %d, remote id %d, data slot size: %d, device fd: %d\n",
	    localid, remid, max_read_size, master_fd));
}

int PortSession::new_session(unsigned char *_remote_node, unsigned char c)
{
    if (!client_session) return -1;

    credit = c;
    strcpy(remote_node, (char *)_remote_node);

    if (master_fd != -1)
    {
	fcntl(master_fd, F_SETFL, fcntl(master_fd, F_GETFL, 0) | O_NONBLOCK);
	connected = true;

	send_login_response();
    }
    else
    {
	return -1;
    }
    return 0;
}

PortSession::~PortSession()
{
    /* Make sure the parent class doesn't close this FD:
       it's not ours. */
    master_fd = -1;

    /* Restart the client session */   
    client_session->restart_pty();
}
