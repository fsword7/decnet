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
#include "portsession.h"

PortSession::PortSession(class LATConnection &p, LAT_SessionStartCmd *cmd,
			 int device_fd,
			 unsigned char remid, unsigned char localid):
  ServerSession(p, cmd, remid, localid)
{
    max_read_size = cmd->dataslotsize;
    master_fd = device_fd;
    
    debuglog(("new port session: localid %d, remote id %d, data slot size: %d, device fd: %d\n",
	    localid, remid, max_read_size, device_fd));
}

int PortSession::new_session(unsigned char *_remote_node, unsigned char c)
{
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
