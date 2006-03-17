/******************************************************************************
    (c) 2000-2003 Patrick Caulfield                 patrick@debian.org

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
#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <termios.h>
#include <list>
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
#include "serversession.h"
#include "reversesession.h"

ReverseSession::ReverseSession(class LATConnection &p, 
			       LAT_SessionStartCmd *cmd,
			       std::string shellcmd,
			       uid_t uid, gid_t gid,
			       unsigned char remid,
			       unsigned char localid, bool clean):
    ServerSession(p, cmd, shellcmd, uid, gid, remid, localid, clean)
{
}


int ReverseSession::new_session(unsigned char *_remote_node,
			       char *service, char *port,
			       unsigned char c)
{
    credit = c;
    strcpy(remote_service, service);
    strcpy(remote_port, port);

    strcpy(remote_node, (char *)_remote_node);

    // Start local command
    return create_session(_remote_node);
}
