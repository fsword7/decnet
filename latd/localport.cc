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
#include <iomanip>

#include "lat.h"
#include "utils.h"
#include "session.h"
#include "connection.h"
#include "circuit.h"
#include "latcpcircuit.h"
#include "localport.h"
#include "server.h"
#include "lat_messages.h"

LocalPort::LocalPort(unsigned char *_service, unsigned char *_portname, 
		     unsigned char *_devname, unsigned char *_remnode,
		     bool _queued, bool _clean):
    service((char*)_service),
    portname((char*)_portname),
    devname((char*)_devname),
    remnode((char*)_remnode),
    queued(_queued),
    clean(_clean),
    slave_fd_open(false),
    connected(false)
{
    debuglog(("New local port %s\n", devname.c_str()));
}

LocalPort::LocalPort(const LocalPort &p)
{
    fprintf(stderr, "LocalPort Copy Constructor\n");
    service = p.service;
    portname = p.portname;
    devname = p.devname;
    remnode = p.remnode;
    queued = p.queued;
    clean = p.clean;
    slave_fd_open = p.slave_fd_open;
    connected = p.connected;
    strcpy(ptyname, p.ptyname);
}


void LocalPort::init_port()
{
// A quick word of explanation here.
// We keep the slave fd open after openpty because otherwise
// the master would go away too (EOF). When the user connects to
// the port we then close the slave fd so that we get EOF when she
// disconnects. got that?

    if (openpty(&master_fd,
		&slave_fd, NULL, NULL, NULL) != 0)
	return;

    // Set terminal characteristics
    struct termios tio;
    tcgetattr(master_fd, &tio);
    tio.c_iflag |= IGNBRK|BRKINT;
    tcsetattr(master_fd, TCSANOW, &tio);

    strcpy(ptyname, ttyname(slave_fd));
    slave_fd_open = true;
    
    // Check for /dev/lat & create it if necessary
    struct stat st;
    if (stat(LAT_DIRECTORY, &st) == -1)
    {
	mkdir(LAT_DIRECTORY, 0755);
    }
    unlink(devname.c_str());
    symlink(ptyname, devname.c_str());

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

    debuglog(("made symlink %s to %s\n", devname.c_str(), ptyname));
    LATServer::Instance()->add_pty(this, master_fd);
    connected = false;
    return;
}

LocalPort::~LocalPort()
{
}

void LocalPort::close_and_delete()
{
    if (slave_fd_open) close (slave_fd);
    close (master_fd);
    LATServer::Instance()->remove_fd(master_fd);
    unlink(devname.c_str());
}

// Disconnect the local PTY
void LocalPort::restart_pty()
{
    debuglog(("LocalPort::restart_pty()\n"));
    connected = false;
    
    // Close it all down so the local side gets EOF
    unlink(devname.c_str());
    
    if (slave_fd_open) close (slave_fd);
    close (master_fd);
    LATServer::Instance()->set_fd_state(master_fd, true);
    LATServer::Instance()->remove_fd(master_fd);
    
    // Now open it all up again ready for a new connection
    init_port();
}


// Remote end disconnects or EOF on local PTY
void LocalPort::disconnect_session(int reason)
{
    debuglog(("LocalPort::disconnect_session()\n"));
    // If the reason was some sort of error then send it to 
    // the PTY
    if (reason >= 1)
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


// Connect up the session
bool LocalPort::connect_session()
{
    return LATServer::Instance()->make_port_connection(master_fd, this, 
						       service.c_str(), remnode.c_str(),
						       portname.c_str(), devname.c_str(),
						       queued);
}


void LocalPort::do_read()
{
    debuglog(("LocalPort::do_read(), connected: %d\n", connected));
    if (!connected)
    {
	if (!connect_session())
	{
	    debuglog(("LocalPort:: do_read disabling pty reads\n"));
	    // Disable reads on the PTY until we are connected (or it fails)
	    LATServer::Instance()->set_fd_state(master_fd, true);

	    close(slave_fd);
	    slave_fd_open = false;
	    connected = true;
	}
	else
	{
	    // Service does not exist or we haven't heard of it yet.
	    restart_pty();
	}
    }
    else
    {
	debuglog(("do_read() called for LocalPort on connected socket\n"));
	// So stop it!
	LATServer::Instance()->set_fd_state(master_fd, true);
    }
}

// Show info for latcp
void LocalPort::show_info(bool verbose, std::ostrstream &output)
{
    output << devname << std::setw(24-devname.length()) << " " << service
	   << std::setw(16-service.length()) << " " 
	   << remnode << std::setw(16-remnode.length()) << " " << portname
	   << std::setw(16-portname.length()) << " " << (queued?"Yes":"No ") 
	   << (clean?" 8":" ") << std::endl;
}
