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
#include <strstream>

#include "lat.h"
#include "utils.h"
#include "session.h"
#include "connection.h"
#include "circuit.h"
#include "latcpcircuit.h"
#include "server.h"
#include "serversession.h"

ServerSession::ServerSession(class LATConnection &p, LAT_SessionStartCmd *cmd,
			     std::string shellcmd,
			     uid_t uid, gid_t gid,
			     unsigned char remid, 
			     unsigned char localid, bool clean):
    LATSession(p, remid, localid, clean),
    command(shellcmd),
    cmd_uid(uid),
    cmd_gid(gid)
{
    max_read_size = cmd->dataslotsize;
    
    
    debuglog(("new server session: localid %d, remote id %d, data slot size: %d\n",
	    localid, remid, max_read_size));


}

int ServerSession::new_session(unsigned char *_remote_node, 
			       char *service, char *port,
			       unsigned char c)
{
    credit = c;
    strcpy(remote_service, service);
    strcpy(remote_port, port);

    strcpy(remote_node, (char *)_remote_node);
    int status = create_session(_remote_node);
    if (status == 0)
    {
	status = send_login_response();
	if (credit) send_issue();
    }
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
    if (credit)
    {
	slotbuf[slotptr++] = 0x26; // Flags
	slotbuf[slotptr++] = 0x13; // Stop  output char XOFF
	slotbuf[slotptr++] = 0x11; // Start output char XON
	slotbuf[slotptr++] = 0x13; // Stop  input char  XOFF
	slotbuf[slotptr++] = 0x11; // Start input char  XON
	
        // data_b slots count against credit

// Hmm, this credit-starves queued connections 
//	add_slot(buf, ptr, 0xaf, slotbuf, slotptr);
//	credit--;
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

	// Become the requested user.
	struct passwd *user_pwd = getpwuid(cmd_uid);
	if (user_pwd)
	    initgroups(user_pwd->pw_name, cmd_gid);
	setgid(cmd_gid);
	setuid(cmd_uid);

	// Get the command to run
	// and do it.
	execute_command(command.c_str());

	// Argh! It returned.
	syslog(LOG_ERR, "Error in starting %s: %m", command.c_str());

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

void ServerSession::execute_command(const char *command)
{
    char cmd[strlen(command)+1];
    strcpy(cmd, command);

    // Count the args
    int num_args = 0;
    strtok(cmd, " ");
    while (strtok(NULL, " ")) num_args++;

    // Make sure we have a clean copy of the command
    strcpy(cmd, command);

    char *argv[num_args+1];
    int   argc = 0;
    const char *cmdname;
    const char *thisarg;
    
    // Gather the args
    thisarg = strtok(cmd, " ");

    // Keep the path for the first arg to execvp
    char fullcmd[strlen(thisarg)+1];
    strcpy(fullcmd, thisarg);

    // For the first arg, remove the path if there is one.
    cmdname = strrchr(thisarg, '/')+1;
    if (cmdname == (char *)1) cmdname = thisarg;

    // We can man malloc all we like here because exec() will
    // "free" it for us !
    do
    {
	if (argc == 0) thisarg = cmdname;

	argv[argc] = (char *)malloc(strlen(thisarg)+1);
	strcpy(argv[argc], thisarg);

	thisarg = strtok(NULL, " ");
	argc++;
    } while (thisarg);

    // Need a NULL at the end of the list
    argv[argc++] = NULL;

    // Set some environment variables. 
    // login will clear these it's true but other 
    // services may find them useful.
    setenv("LAT_LOCAL_SERVICE", parent.get_servicename(), 1);
    setenv("LAT_REMOTE_NODE", remote_node, 1);
    setenv("LAT_REMOTE_PORT", remote_port, 1);

    // Run it.
    execvp(fullcmd, argv);
}
