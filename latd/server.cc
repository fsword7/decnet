/******************************************************************************
    (c) 2001-2004 Patrick Caulfield                 patrick@debian.org
    (c) 2003 Dmitri Popov

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
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <syslog.h>
#include <ctype.h>
#include <regex.h>
#include <stdlib.h>
#include <utmp.h>
#include <grp.h>
#include <signal.h>
#include <assert.h>
#include <netinet/in.h>

#include <list>
#include <queue>
#include <map>
#include <string>
#include <algo.h>
#include <iterator>
#include <strstream>
#include <iomanip>

#include "lat.h"
#include "utils.h"
#include "session.h"
#include "localport.h"
#include "connection.h"
#include "circuit.h"
#include "latcpcircuit.h"
#include "llogincircuit.h"
#include "server.h"
#include "services.h"
#include "lat_messages.h"
#include "dn_endian.h"

#ifdef __APPLE__
typedef int socklen_t;
#endif

// Remove any dangling symlinks in the /dev/lat directory
void LATServer::tidy_dev_directory()
{
    DIR *dir;
    struct dirent *de;
    char current_dir[PATH_MAX];

    getcwd(current_dir, sizeof(current_dir));
    chdir(LAT_DIRECTORY);

    dir = opendir(LAT_DIRECTORY);
    if (!dir) return; // Doesn't exist - this is OK

    while ( (de = readdir(dir)) )
    {
	if (de->d_name[0] != '.')
	    unlink(de->d_name);
    }
    closedir(dir);
    chdir(current_dir);
}

unsigned char *LATServer::get_local_node(void)
{
    unsigned int i;

    if (local_name[0] == '\0')
    {
	struct utsname uts;
	uname(&uts);
	if (strchr(uts.nodename, '.'))
	    *strchr(uts.nodename, '.') = '\0';

        // This gets rid of the name-Computer bit on Darwin
	if (strchr(uts.nodename, '-'))
	    *strchr(uts.nodename, '-') = '\0';

	// LAT node names must be <= 16 characters long
	if (strlen(uts.nodename) > 16)
	    uts.nodename[16] = '\0';

	strcpy((char *)local_name, uts.nodename);

	// Make it all upper case
	for (i=0; i<strlen((char *)local_name); i++)
	{
	    if (islower(local_name[i])) local_name[i] = toupper(local_name[i]);
	}
    }
    return local_name;
}

void LATServer::alarm_signal(int sig)
{
     if (Instance()->alarm_mode == 0)
     {
	 Instance()->send_service_announcement(sig);
     }
     Instance()->send_solicit_messages(sig);

}


// Send ENQUIRY for a node - mainly needed for DS90L servers that
// don't advertise.
void LATServer::send_enq(const char *node)
{
    unsigned char packet[1600];
    static unsigned char id = 1;
    LAT_Enquiry *enqmsg = (LAT_Enquiry *)packet;
    int ptr = sizeof(LAT_Enquiry);

    enqmsg->cmd = LAT_CCMD_ENQUIRE;
    enqmsg->dummy = 0;
    enqmsg->hiver = LAT_VERSION;
    enqmsg->lover = LAT_VERSION;
    enqmsg->latver = LAT_VERSION;
    enqmsg->latver_eco = LAT_VERSION_ECO;
    enqmsg->mtu = dn_htons(1500);
    enqmsg->id  = id++;
    enqmsg->retrans_timer = 75; // * 10ms - give it more time to respond

    add_string(packet, &ptr, (const unsigned char *)node);
    packet[ptr++] = 1; /* Length of group data */
    packet[ptr++] = 1; /* Group mask */

    add_string(packet, &ptr, local_name);
    packet[ptr++] = 0x00;
    packet[ptr++] = 0x00;
    packet[ptr++] = 0x00;
    packet[ptr++] = 0x00;

    /* This is the LAT multicast address */
    static unsigned char addr[6] = { 0x09, 0x00, 0x2b, 0x00, 0x00, 0x0f };

debuglog(("send_enq : node: %s, local_name: %s\n", node, local_name));

    for (int i=0; i<num_interfaces;i++)
    {
	if (iface->send_packet(interface_num[i], addr, packet, ptr) < 0)
	{
debuglog(("Error send packet\n"));
	    interface_error(interface_num[i], errno);
	}
	else
	{
	    interface_errs[interface_num[i]] = 0; // Clear errors
	}
    }

    LATServices::Instance()->touch_dummy_node_respond_counter(node);
}

void LATServer::add_slave_node(const char *node_name)
{
    sig_blk_t _block(SIGALRM);
    for (std::list<std::string>::iterator iter = slave_nodes.begin();
         iter != slave_nodes.end();
         iter++)
    {
        if (*iter == node_name) {
           // do not duplicate nodes
           return;
        }
    }
    slave_nodes.push_front(node_name);
}

/* Called on the multicast timer - advertise our service on the LAN */
void LATServer::send_service_announcement(int sig)
{
    unsigned char packet[1600];
    int ptr;
    struct utsname uinfo;
    char  *myname;

    // Only send it if we have some services
    if (servicelist.size())
    {
	LAT_ServiceAnnounce *announce = (LAT_ServiceAnnounce *)packet;
	ptr = sizeof(LAT_ServiceAnnounce);

	announce->cmd             = LAT_CCMD_SERVICE;
	announce->circuit_timer   = circuit_timer;
	announce->hiver           = LAT_VERSION;
	announce->lover           = LAT_VERSION;
	announce->latver          = LAT_VERSION;
	announce->latver_eco      = LAT_VERSION_ECO;
	announce->incarnation     = --multicast_incarnation;
	announce->flags           = 0x1f;
	announce->mtu             = dn_htons(1500);
	announce->multicast_timer = multicast_timer;
	if (do_shutdown)
	{
	    announce->node_status     = 3;    // Not accepting connections
	}
	else
	{
	    announce->node_status     = 2;    // Accepting connections
	}

	// Send group codes
	if (groups_set)
	{
	    announce->group_length = 32;
	    memcpy(&packet[ptr], groups, 32);
	    ptr += 32;
	    announce->flags |= 1;
	}
	else
	{
	    announce->group_length    = 1;
	    packet[ptr++] = 01;
	}

	/* Get host info */
	uname(&uinfo);

	// Node name
	myname = (char*)get_local_node();
	packet[ptr++] = strlen(myname);
	strcpy((char*)packet+ptr, myname);
	ptr += strlen(myname);

	// Greeting
	packet[ptr++] = strlen((char*)greeting);
	strcpy((char*)packet+ptr, (char*)greeting);
	ptr += strlen((char*)greeting);

	// Number of services
	packet[ptr++] = servicelist.size();
	std::list<serviceinfo>::iterator i(servicelist.begin());
	for (; i != servicelist.end(); i++)
	{
	    // Service rating
	    unsigned char real_rating;
	    if (i->get_static())
	    {
		real_rating = i->get_rating();
	    }
	    else
	    {
		/* Calculate dynamic rating */
		real_rating =  (unsigned char)(i->get_rating() / (get_loadavg()+1.0));
		debuglog(("Dynamic service rating is %d\n", real_rating));
	    }
	    packet[ptr++] = real_rating;

	    // Service name
	    const std::string name = i->get_name();
	    packet[ptr++]     = name.length();
	    strcpy((char *)packet+ptr, i->get_name().c_str());
	    ptr += name.length();

	    // Service Identification
	    std::string id = i->get_id();
	    if (id.length() == 0)
	    {
		// Default service identification string
		char stringbuf[1024];
		sprintf(stringbuf, "%s %s", uinfo.sysname, uinfo.release);
		id = std::string(stringbuf);
	    }

	    packet[ptr++] = id.length();
	    strcpy((char *)packet+ptr, id.c_str());
	    ptr += id.length();

	    // Make sure the service table knows about all our services
	    unsigned char dummy_macaddr[6];
	    memset(dummy_macaddr, 0, sizeof(dummy_macaddr));
	    if (!do_shutdown)
		LATServices::Instance()->add_service(std::string((char*)get_local_node()),
						     name,
						     id, real_rating, 0, dummy_macaddr);
	}

	// Not sure what node service classes are
	// probably somthing to do with port services and stuff.
	packet[ptr++] = 0x01; // Node service classes length
	packet[ptr++] = 0x01; // Node service classes
	packet[ptr++] = 0x00;
	packet[ptr++] = 0x00;

	unsigned char addr[6];
	/* This is the LAT multicast address */
	addr[0]  = 0x09;
	addr[1]  = 0x00;
	addr[2]  = 0x2b;
	addr[3]  = 0x00;
	addr[4]  = 0x00;
	addr[5]  = 0x0f;

	for (int i=0; i<num_interfaces;i++)
	{
	    if (iface->send_packet(interface_num[i], addr, packet, ptr) < 0)
	    {
		debuglog(("sending service announcement, send error: %d\n", errno));
		interface_error(interface_num[i], errno);
	    }
	    else
		interface_errs[interface_num[i]] = 0; // Clear errors
	}
    }
    /* Send it every minute */
    signal(SIGALRM, &alarm_signal);
    alarm(multicast_timer);
}

// Send solicit messages to slave nodes
void LATServer::send_solicit_messages(int sig)
{
    static int counter = 0;
    static int last_list_size = 0;
    if (alarm_mode == 0)
    {
	counter = 0;
	alarm_mode = 1;
	debuglog(("set alarm_mode to 1\n"));

	if (!known_slave_nodes.empty())
	{
	    std::string known_node = known_slave_nodes.front();
	    known_slave_nodes.pop_front();
	    slave_nodes.push_back(known_node);
	    debuglog(("known(%d) => slave(%d) : %s\n", known_slave_nodes.size(),
		       slave_nodes.size(), known_node.c_str()));
	    if (slave_nodes.size() == 1)
	    {
		alarm_mode = 0;
		debuglog(("set alarm_mode to 0 - one slave\n"));
		send_enq(slave_nodes.front().c_str());
		// alarm() is already charged by send_service_announcement()
		return;
	    }
	}
    }
    else
    {
	if (slave_nodes.size() < last_list_size)
	{
	    counter -= last_list_size - slave_nodes.size();
	}
    }

    if (slave_nodes.size() > counter && !slave_nodes.empty())
    {
	std::string node_name = slave_nodes.front();
	slave_nodes.pop_front();
	slave_nodes.push_back(node_name);

	send_enq(node_name.c_str());

	alarm(1);
	counter++;
	last_list_size = slave_nodes.size();
    }
    else
    {
	alarm_mode = 0;

	//well, it's not quite correct, I know...
	alarm(counter >= multicast_timer ? 1 : multicast_timer - counter);
	debuglog(("set alarm_mode to 0, timer %d\n", counter >= multicast_timer ? 1 : multicast_timer - counter));
    }
}

// Log an error against an interface. If we get three of these
// then we remove it.
// If that means we have no interfaces, then closedown.
void LATServer::interface_error(int ifnum, int err)
{
    syslog(LOG_ERR, "Error on interface %s: %s\n", iface->ifname(ifnum).c_str(), strerror(err));

    // Too many errors, remove it
    if (++interface_errs[ifnum] > 3)
    {
	int i,j;

	syslog(LOG_ERR, "Interface will be removed from LATD\n");

	// Look for it
	for (i=0; i<num_interfaces; i++)
	{
	    if (interface_num[i] == ifnum) goto remove_if;
	}

	// Ugh, didn't find it. that can't be right!
	if (i>=num_interfaces)
	{
	    syslog(LOG_ERR, "Don't seem to have a reference to this interface....\n");
	    return;
	}

    remove_if:
	// Shuffle the list down
	for (j=i; j<num_interfaces-1; j++)
	{
	    interface_num[j] = interface_num[j+1];
	}

	if (--num_interfaces == 0)
	{
	    syslog(LOG_ERR, "No valid interfaces left. LATD closing down\n");

	    // No point in going down cleanly as that just sends network messages
	    // and we have nowhere to send then through.
	    exit(9);
	}
    }
}

/* Main loop */
void LATServer::run()
{
    int status;

    // Bind interfaces
    for (int i=0; i<num_interfaces;i++)
    {
	iface->set_lat_multicast(interface_num[i]);
    }

    // Add it/them to the sockets list
    if (iface->one_fd_per_interface())
    {
	for (int i=0; i<num_interfaces;i++)
	{
	    fdlist.push_back(fdinfo(iface->get_fd(interface_num[i]), 0, LAT_SOCKET));
	}
    }
    else
    {
	fdlist.push_back(fdinfo(iface->get_fd(0), 0, LAT_SOCKET));
    }

    // Open LATCP socket
    unlink(LATCP_SOCKNAME);
    latcp_socket = socket(PF_UNIX, SOCK_STREAM, 0);
    if (latcp_socket < 0)
    {
	syslog(LOG_ERR, "Can't create latcp socket: %m");
	exit(1);
    }

    struct sockaddr_un sockaddr;
    strcpy(sockaddr.sun_path, LATCP_SOCKNAME);
    sockaddr.sun_family = AF_UNIX;
    if (bind(latcp_socket, (struct sockaddr *)&sockaddr, sizeof(sockaddr)))
    {
	syslog(LOG_ERR, "can't bind latcp socket: %m");
        exit(1);
    }
    if (listen(latcp_socket, 1) != 0)
    {
	syslog(LOG_ERR, "listen latcp: %m");
    }
    // Make sure only root can talk to us via the latcp socket
    chmod(LATCP_SOCKNAME, 0600);
    fdlist.push_back(fdinfo(latcp_socket, 0, LATCP_RENDEZVOUS));

    // Open llogin socket
    unlink(LLOGIN_SOCKNAME);
    llogin_socket = socket(PF_UNIX, SOCK_STREAM, 0);
    if (llogin_socket < 0)
    {
	syslog(LOG_ERR, "Can't create llogin socket: %m");
	exit(1);
    }

    strcpy(sockaddr.sun_path, LLOGIN_SOCKNAME);
    sockaddr.sun_family = AF_UNIX;
    if (bind(llogin_socket, (struct sockaddr *)&sockaddr, sizeof(sockaddr)))
    {
	syslog(LOG_ERR, "can't bind llogin socket: %m");
        exit(1);
    }
    if (listen(llogin_socket, 1) != 0)
    {
	syslog(LOG_ERR, "listen llogin: %m");
    }
    // Make sure everyone can use it
    chmod(LLOGIN_SOCKNAME, 0666);
    fdlist.push_back(fdinfo(llogin_socket, 0, LLOGIN_RENDEZVOUS));

    // Don't start sending service announcements
    // until we get an UNLOCK message from latcp.

    // We rely on POSIX's select() behaviour in that we use the
    // time left in the timeval parameter to make sure the circuit timer
    // goes off at a reasonably predictable interval.

#ifndef __linux__
    struct timeval next_circuit_time;
    next_circuit_time.tv_sec = 0;
#endif

    struct timeval tv = {0, circuit_timer*10000};
    time_t last_node_expiry = time(NULL);
    do_shutdown = false;
    do
    {
	fd_set fds;
	FD_ZERO(&fds);

	std::list<fdinfo>::iterator i(fdlist.begin());
	for (; i != fdlist.end(); i++)
	{
	    if (i->active())
	        FD_SET(i->get_fd(), &fds);
	}

// Only Linux seems to have this select behaviour...
// for BSDs we need to work our for ourselves the delay
// until the next circuit timer tick.
#ifndef __linux__
	struct timeval now;

	gettimeofday(&now, NULL);
	if (next_circuit_time.tv_sec == 0)
	{
	    next_circuit_time = now;
	    next_circuit_time.tv_usec += circuit_timer*10000;
	    next_circuit_time.tv_sec += (next_circuit_time.tv_usec / 1000000);
	    next_circuit_time.tv_usec = (next_circuit_time.tv_usec % 1000000);
	}

	tv = next_circuit_time;
	if (tv.tv_sec < now.tv_sec
	    || (tv.tv_sec == now.tv_sec
		&& tv.tv_usec <= now.tv_usec))
	{
	    tv.tv_sec = tv.tv_usec = 0;
	}
	else
	{
	    tv.tv_sec -= now.tv_sec;
	    if ((tv.tv_usec -= now.tv_usec) < 0)
	    {
		tv.tv_sec--;
		tv.tv_usec += 1000000;
	    }
	}
#endif

	status = select(FD_SETSIZE, &fds, NULL, NULL, &tv);
	if (status < 0)
	{
	    if (errno != EINTR)
	    {
		syslog(LOG_WARNING, "Error in select: %m");
		debuglog(("Error in select: %s\n", strerror(errno)));
		do_shutdown = true;
	    }
	}
	else
	{
	    if (status == 0) // Circuit timer
	    {
	      for (int i=1; i<MAX_CONNECTIONS; i++)
		if (connections[i])
		  connections[i]->circuit_timer();
	      tv.tv_usec = circuit_timer*10000;

	      // Is it time we expired some more nodes ?
	      if (time(NULL) - last_node_expiry > 15)
		  LATServices::Instance()->expire_nodes();
#ifndef __linux__
	      next_circuit_time.tv_sec = 0;
#endif
	    }
	    else
	    {
		// Unix will never scale while this is necessary
		std::list<fdinfo>::iterator fdl(fdlist.begin());
		for (; fdl != fdlist.end(); fdl++)
		{
		    if (fdl->active() &&
			FD_ISSET(fdl->get_fd(), &fds))
		    {
			process_data(*fdl);
		    }
		}
	    }
	}

	// Tidy deleted sessions
	if (!dead_session_list.empty())
	{
	    std::list<deleted_session>::iterator dsl(dead_session_list.begin());
	    for (; dsl != dead_session_list.end(); dsl++)
	    {
		delete_entry(*dsl);
	    }
	    dead_session_list.clear();
	}

	// Tidy deleted connections
	if (!dead_connection_list.empty())
	{
	    std::list<int>::iterator dcl(dead_connection_list.begin());
	    for (; dcl != dead_connection_list.end(); dcl++)
	    {
		if (connections[*dcl])
	        {
		    delete connections[*dcl];
		    connections[*dcl] = NULL;
		}
	    }
	    dead_connection_list.clear();
	}

    } while (!do_shutdown);

    send_service_announcement(-1); // Say we are unavailable

    close(latcp_socket);
    unlink(LATCP_SOCKNAME);
    unlink(LLOGIN_SOCKNAME);

    tidy_dev_directory();
}

/* LAT socket has something for us */
void LATServer::read_lat(int sock)
{
    unsigned char buf[1600];
    unsigned char macaddr[6];
    int    len;
    int    i;
    int    ifn;
    bool   more = true;
    LAT_Header *header = (LAT_Header *)buf;

    while (more)
    {
	len = iface->recv_packet(sock, ifn, macaddr, buf, sizeof(buf), more);
	if (len == 0)
	    continue; // Probably a rogue packet

	if (len < 0)
	{
	    if (errno != EINTR && errno != EAGAIN)
	    {
		syslog(LOG_ERR, "recvmsg: %m");
		return;
	    }
	}

	// Not listening yet, but we must read the message otherwise we
	// we will spin until latcp unlocks us.
	if (locked)
	       continue;

	// Parse & dispatch it.
	switch(header->cmd)
	{
	case LAT_CCMD_SREPLY:
	case LAT_CCMD_SDATA:
	case LAT_CCMD_SESSION:
        {
	    debuglog(("session cmd for connid %d\n", header->remote_connid));
	    LATConnection *conn = NULL;
            if(header->remote_connid < MAX_CONNECTIONS)
            {
                conn = connections[header->remote_connid];
            }

	    if (conn)
	    {
	        conn->process_session_cmd(buf, len, macaddr);
	    }
	    else
	    {
		// Message format error
		send_connect_error(2, header, ifn, macaddr);
	    }
	}
	break;

	case LAT_CCMD_CONNECT:
        {
	    // Make a new connection

	    //  Check that the connection is really for one of our services
	    unsigned char name[256];
	    int ptr = sizeof(LAT_Start);
	    get_string(buf, &ptr, name);

	    debuglog(("got connect for node %s\n", name));

	    if (strcmp((char *)name, (char *)get_local_node()))
	    {
		// How the &?* did that happen?
		send_connect_error(2, header, ifn, macaddr);
		continue;
	    }

     	    // Make a new connection.
	    if ( ((i=make_new_connection(buf, len, ifn, header, macaddr) )) > 0)
	    {
		debuglog(("Made new connection: %d\n", i));
		connections[i]->send_connect_ack();
	    }
	}
	break;

	case LAT_CCMD_CONACK:
        {
	    LATConnection *conn = NULL;
	    debuglog(("Got connect ACK for %d\n", header->remote_connid));
	    if (header->remote_connid < MAX_CONNECTIONS)
	    {
		conn = connections[header->remote_connid];
	    }

	    if (conn)
	    {
		conn->got_connect_ack(buf);
	    }
	    else
	    {
		// Insufficient resources
		send_connect_error(7, header, ifn, macaddr);
	    }
	}
	break;

	case LAT_CCMD_CONREF:
	case LAT_CCMD_DISCON:
        {
	    debuglog(("Disconnecting connection %d: status %x(%s)\n",
		      header->remote_connid,
		      buf[sizeof(LAT_Header)],
		      lat_messages::connection_disconnect_msg(buf[sizeof(LAT_Header)]) ));
	    if (header->remote_connid < MAX_CONNECTIONS)
	    {
		LATConnection *conn = connections[header->remote_connid];
		if (conn)
		{
		    // We don't delete clients, we just quiesce them.
		    if (conn->isClient())
		    {
			conn->disconnect_client();
			if (conn->num_clients() == 0)
			{
			    delete conn;
			    connections[header->remote_connid] = NULL;
			}
		    }
		    else
		    {
			delete conn;
			connections[header->remote_connid] = NULL;
		    }
		}
	    }
	    else
	    {
		// Message format error
		send_connect_error(2, header, ifn, macaddr);
	    }
	}
	break;

	case LAT_CCMD_SERVICE:
	    // Keep a list of known services
	    add_services(buf, len, ifn, macaddr);
	    break;

	case LAT_CCMD_ENQUIRE:
	    reply_to_enq(buf, len, ifn, macaddr);
	    break;

	case LAT_CCMD_ENQREPLY:
	    got_enqreply(buf, len, ifn, macaddr);
	    break;

	case LAT_CCMD_STATUS:
	    forward_status_messages(buf, len);
	    break;

	    // Request for a reverse-LAT connection.
	case LAT_CCMD_COMMAND:
	    process_command_msg(buf, len, ifn, macaddr);
	    break;
	}
    }
}

// Add an FD to the list of FDs to listen to
void LATServer::add_fd(int fd, fd_type type)
{
    std::list<fdinfo>::iterator fdi;

    debuglog(("Add_fd: %d\n", fd));

    fdi = find(fdlist.begin(), fdlist.end(), fd);
    if (fdi != fdlist.end()) return; // Already exists


    fdlist.push_back(fdinfo(fdinfo(fd, NULL, type)));
}

// Remove FD from the FD list
void LATServer::remove_fd(int fd)
{
    std::list<fdinfo>::iterator fdi;
    debuglog(("remove_fd: %d\n", fd));

    fdi = find(fdlist.begin(), fdlist.end(), fd);
    if (fdi == fdlist.end()) return; // Does not exist

    fdlist.remove(*fdi);
}

// Change the DISABLED state of a PTY fd
void LATServer::set_fd_state(int fd, bool disabled)
{
    std::list<fdinfo>::iterator fdi;
    debuglog(("set_fd_state: %d, %d\n", fd, disabled));

    fdi = find(fdlist.begin(), fdlist.end(), fd);
    if (fdi == fdlist.end()) return; // Does not exist

    fdi->set_disabled(disabled);
}



/* Send a LAT message to a specified MAC address */
int LATServer::send_message(unsigned char *buf, int len, int interface, unsigned char *macaddr)
{
  if (len < 46) len = 46; // Minimum packet length
  if (len%2) len++;       // Must be an even number

#ifdef PACKET_LOSS
  static int c=0;
  if ((++c % PACKET_LOSS) == 0) {
      debuglog(("NOT SENDING THIS MESSAGE!\n"));
      return 0;
  }
#endif

  if (interface == 0) // Send to all
  {
      for (int i=0; i<num_interfaces;i++)
      {
	  if (iface->send_packet(interface_num[i], macaddr, buf, len) < 0)
	  {
	      interface_error(interface_num[i], errno);
	  }
	  else
	      interface_errs[interface_num[i]] = 0; // Clear errors

      }
  }
  else
  {
      if (iface->send_packet(interface, macaddr, buf, len) < 0)
      {
	  interface_error(interface, errno);
	  return -1;
      }
      interface_errs[interface] = 0; // Clear errors
  }

  return 0;

}

/* Get the system load average */
double LATServer::get_loadavg(void)
{
    double avg[3];

    if (getloadavg(avg, 3) > 0)
    {
	return avg[0];
    }
    else
    {
	return 0;
    }
}

// Forward status messages to their recipient connection objects.
void LATServer::forward_status_messages(unsigned char *inbuf, int len)
{
    int ptr = sizeof(LAT_Status);
    unsigned char node[256];

    get_string(inbuf, &ptr, node);

    // Forward all the StatusEntry messages
    while (ptr <= len)
    {
	if (ptr%2) ptr++; // Word aligned

	LAT_StatusEntry *entry = (LAT_StatusEntry *)&inbuf[ptr];
	if (entry->length == 0) break;

	ptr += sizeof(LAT_StatusEntry);
	get_string(inbuf, &ptr, node); // Service name
	ptr += inbuf[ptr]+1; // Past port name
	ptr += inbuf[ptr]+1; // Past service description

	if (entry->request_id < MAX_CONNECTIONS &&
	    connections[entry->request_id])
	    connections[entry->request_id]->got_status(node, entry);

    }
}

/* Reply to an ENQUIRE message with our MAC address & service name */
void LATServer::reply_to_enq(unsigned char *inbuf, int len, int interface,
			     unsigned char *remote_mac)
{
    int inptr, outptr, i;
    unsigned char outbuf[1600];
    unsigned char req_service[1600];
    LAT_Header *outhead = (LAT_Header *)outbuf;
    LAT_Header *inhead = (LAT_Header *)inbuf;

    inptr = sizeof(LAT_Header)+4;

    /* This is the service being enquired about */
    get_string(inbuf, &inptr, req_service);

    debuglog(("got ENQ for %s\n", req_service));

    // Ignore empty requests:: TODO: is this right?? - maybe we issue
    // a response with all our services in it.
    if (strlen((char*)req_service) == 0) return;

    unsigned char reply_macaddr[6];
    unsigned char *reply_node;

    // See if it is for us if we're not doing responder work.
    if (!responder)
    {
	std::list<serviceinfo>::iterator sii;
	sii = find(servicelist.begin(), servicelist.end(), (char *)req_service);
	if (sii == servicelist.end()) return; // Not ours
    }

    std::string node;
    if (LATServices::Instance()->get_highest(std::string((char *)req_service),
					     node, reply_macaddr, &interface))
    {
	reply_node = (unsigned char *)node.c_str();
    }
    else
    {
	return; // nevver 'eard of 'im.
    }

    debuglog(("Sending ENQ reply for %s\n", req_service));

    inptr += 2;

    outptr = sizeof(LAT_Header);
    outbuf[outptr++] = 0x93;
    outbuf[outptr++] = 0x07;
    outbuf[outptr++] = 0x00;
    outbuf[outptr++] = 0x00;

    /* The service's MAC address */
    outbuf[outptr++] = 0x06;
    outbuf[outptr++] = 0x00;
    for (i=0; i<5; i++)
	outbuf[outptr++] = reply_macaddr[i];

    /* Don't know what this is */
    outbuf[outptr++] = 0x14;
    outbuf[outptr++] = 0x00;

    add_string(outbuf, &outptr, reply_node);
    outbuf[outptr++] = 0x01;
    outbuf[outptr++] = 0x01;
    add_string(outbuf, &outptr, req_service);

    outhead->cmd             = LAT_CCMD_ENQREPLY;
    outhead->num_slots       = 0;
    outhead->remote_connid   = inhead->remote_connid;
    outhead->local_connid    = inhead->local_connid;
    outhead->sequence_number = inhead->sequence_number;
    outhead->ack_number      = inhead->ack_number;

    send_message(outbuf, outptr, interface, remote_mac);
}

void LATServer::shutdown()
{
    do_shutdown = true;
}

void LATServer::init(bool _static_rating, int _rating,
		     char *_service, char *_greeting, char **_interfaces,
		     int _verbosity, int _timer)
{
    // Server is locked until latcp has finished
    locked = true;

    /* Initialise the platform-specific interface code */
    iface = LATinterfaces::Create();
    if (iface->Start(LATinterfaces::ProtoLAT) == -1)
    {
	syslog(LOG_ERR, "Can't create LAT protocol socket: %m\n");
	exit(1);

    }

#ifdef ENABLE_DEFAULT_SERVICE
    // Add the default session
    servicelist.push_back(serviceinfo(_service,
				      _rating,
				      _static_rating));
#endif

    strcpy((char *)greeting, _greeting);
    verbosity = _verbosity;

    // Convert all the interface names to numbers and check they are usable
    if (_interfaces[0])
    {
	int i=0;
	while (_interfaces[i])
	{
	    interface_num[num_interfaces] = iface->find_interface(_interfaces[i]);
	    if (interface_num[num_interfaces] == -1)
	    {
		syslog(LOG_ERR, "Can't use interface %s: ignored\n", _interfaces[i]);
	    }
	    else
	    {
		interface_errs[num_interfaces] = 0;  // Clear errors
		num_interfaces++;
	    }
	    i++;
	}
	if (num_interfaces == 0)
	{
	    syslog(LOG_ERR, "No usable interfaces, latd exiting.\n");
	    exit(9);
	}
    }
    else
    {
	iface->get_all_interfaces(interface_num, num_interfaces);
    }

    // Remove any old /dev/lat symlinks
    tidy_dev_directory();

    // Save these two for any newly added services
    rating = _rating;
    static_rating = _static_rating;

    next_connection = 1;
    multicast_incarnation = 0;
    circuit_timer = _timer/10;
    local_name[0] = '\0'; // Use default node name

    memset(connections, 0, sizeof(connections));

    // Enable user group 0
    memset(user_groups, 0, 32);
    user_groups[0] = 1;

    // Look in /etc/group for a group called "lat"
    struct group *gr = getgrnam("lat");
    if (gr)
	lat_group = gr->gr_gid;
    else
	lat_group = 0;

}

// Create a new connection object for this remote node.
int LATServer::make_new_connection(unsigned char *buf, int len,
				   int interface,
				   LAT_Header *header,
				   unsigned char *macaddr)
{
    int i;
    i = get_next_connection_number();

    if (i >= 0)
    {
	next_connection = i+1;
	connections[i] = new LATConnection(i, buf, len, interface,
					   header->sequence_number,
					   header->ack_number,
					   macaddr);
    }
    else
    {
// Number of virtual circuits exceeded
	send_connect_error(9, header, interface, macaddr);
	return -1;
    }
    return i;
}

void LATServer::delete_session(int conn, unsigned char id, int fd)
{
    // Add this to the list and delete them later
    deleted_session s(LOCAL_PTY, conn, id, fd);
    dead_session_list.push_back(s);
}

void LATServer::delete_connection(int conn)
{
    // Add this to the list and delete them later
    debuglog(("connection %d pending deletion\n", conn));

    dead_connection_list.push_back(conn);
}

// Got a reply from a DS90L - add it to the services list in a dummy service
void LATServer::got_enqreply(unsigned char *buf, int len, int interface, unsigned char *macaddr)
{
    int off = 14;
    unsigned char node_addr[6];
    char node_description[256];
    char node_name[256];

    memset(node_addr, 0x00, sizeof(node_addr));
    if (0 == memcmp(node_addr, buf + off, sizeof(node_addr)))
    {
	return;
    }

    memset(node_addr, 0xFF, sizeof(node_addr));
    if (0 == memcmp(node_addr, buf + off, sizeof(node_addr)))
    {
	return;
    }

    // Node MAC address
    memcpy(node_addr, buf + off, sizeof(node_addr));

    // Skip destination node name
    off = 22;
    off += buf[off] + 1;

    // Skip node groups
    off += buf[off] + 1;

    get_string(buf, &off, (unsigned char*)node_name);
    get_string(buf, &off, (unsigned char*)node_description);

#if defined(debuglog)
    if (memcmp(node_addr, macaddr, sizeof(node_addr))) {
	debuglog(("got_enqreply : macaddr is different : %02hhX-%02hhX-%02hhX-%02hhX-%02hhX-%02hhX\n",
		  macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]));
    }
#endif

// Add it to the dummy service.
    debuglog(("got_enqreply : node: '%s' : %02hhX-%02hhX-%02hhX-%02hhX-%02hhX-%02hhX\n",
	      node_name,
	      node_addr[0], node_addr[1], node_addr[2], node_addr[3], node_addr[4], node_addr[5]));
    LATServices::Instance()->add_service(std::string(node_name),
					 std::string(""), // Dummy service name
					 std::string(node_description),
					 0,
					 interface, node_addr);

    {
        sig_blk_t _block(SIGALRM);
	std::list<std::string>::iterator sl_iter;
        sl_iter = find(slave_nodes.begin(), slave_nodes.end(), node_name);
        if (sl_iter != slave_nodes.end()) {
            debuglog(("got_enqreply : to remove from slave(%d), node: '%s'\n", 
		      slave_nodes.size(), sl_iter->c_str()));
	    slave_nodes.erase(sl_iter);
	}

        sl_iter = find(known_slave_nodes.begin(), known_slave_nodes.end(), node_name);
        if (sl_iter == known_slave_nodes.end()) {
	    known_slave_nodes.push_back(node_name);
            debuglog(("got_enqreply : added to known(%d), node: '%s'\n", 
		      known_slave_nodes.size(),
		       node_name));
	}
    }
}


// Got a COMMAND message from a remote node. it probably wants
// to set up a reverse-LAT session to us.
void LATServer::process_command_msg(unsigned char *buf, int len, int interface, unsigned char *macaddr)
{
    LAT_Command *msg = (LAT_Command *)buf;
    int ptr = sizeof(LAT_Command);
    unsigned char remnode[256];
    unsigned char remport[256];
    unsigned char service[256];
    unsigned char portname[256];

// TODO: Check the params in the LAT_Command message...

    ptr += buf[ptr++]; // Skip past groups for now.
    ptr += buf[ptr++]; // Skip past groups for now.

    // Get the remote node name.
    get_string(buf, &ptr, remnode);
    get_string(buf, &ptr, remport);
    ptr+= buf[ptr++]; // Skip past greeting...
    get_string(buf, &ptr, service);
    get_string(buf, &ptr, portname);

    debuglog(("Got request for reverse-LAT connection to %s/%s from %s/%s\n",
	      service, portname, remnode, remport));

    // Make a new connection
    // We can reuse connections here...
    int connid = find_connection_by_node((char *)remnode);
    if (connid == -1)
    {
	connid = get_next_connection_number();
	connections[connid] = new LATConnection(connid,
						(char *)service,
						(char *)remport,
						(char *)portname,
						(char *)remnode,
						false,
						true);
    }

    // Make a reverse-session and connect it.
    LAT_SessionStartCmd startcmd;
    memset(&startcmd, 0, sizeof(startcmd));
    startcmd.dataslotsize = 255;
    if (connections[connid]->create_reverse_session((const char *)service,
						    (const char *)&startcmd,
						    dn_ntohs(msg->request_id),
						    interface, macaddr) == -1)
    {
	// If we created this connection just for the new session
	// then get rid of it.
	if (connections[connid]->num_clients() == 0)
	    delete_connection(connid);
    }
}

// Add services received from a service announcement multicast
void LATServer::add_services(unsigned char *buf, int len, int interface, unsigned char *macaddr)
{
    LAT_ServiceAnnounce *announce = (LAT_ServiceAnnounce *)buf;
    int ptr = sizeof(LAT_ServiceAnnounce);
    unsigned char service_groups[32];
    unsigned char nodename[256];
    unsigned char greeting[255];
    unsigned char service[255];
    unsigned char ident[255];

    // Set the groups to all zeros initially
    memset(service_groups, 0, sizeof(service_groups));

    // Make sure someone isn't pulling our leg.
    if (announce->group_length > 32)
	announce->group_length = 32;

    // Get group numbers
    memcpy(service_groups, buf+ptr, announce->group_length);

    // Compare with our user groups mask (which is always either completely
    // empty or the full 32 bytes)
    int i;
    bool gotone = false;
    for (i=0; i<announce->group_length; i++)
    {
	if (user_groups[i] & service_groups[i])
	{
	    gotone = true;
	    break;
	}
    }

    if (!gotone)
    {
	debuglog(("remote node not in our user groups list\n"));
	return;
    }

    ptr += announce->group_length;
    get_string(buf, &ptr, nodename);
    get_string(buf, &ptr, greeting);

    int num_services = buf[ptr++];

    debuglog(("service announcement: status = %d\n", announce->node_status));

    // Get all the services & register them
    for (int i=1; i<=num_services; i++)
    {
	int rating       = buf[ptr++];

	get_string(buf, &ptr, service);
	get_string(buf, &ptr, ident);

	if ( announce->node_status%1 == 0)
	{
	    LATServices::Instance()->add_service(std::string((char*)nodename),
						 std::string((char*)service),
						 std::string((char*)ident),
						 rating,
						 interface, macaddr);
	}
	else
	{
	    LATServices::Instance()->remove_node(std::string((char*)nodename));
	}
    }
}

// Wait for data available on a client PTY
void LATServer::add_pty(LocalPort *port, int fd)
{
    fdlist.push_back(fdinfo(fd, port, LOCAL_PTY));
}


// Got a new connection from latcp
void LATServer::accept_latcp(int fd)
{
    struct sockaddr_un socka;
    socklen_t sl = sizeof(socka);

    int latcp_client_fd = accept(fd, (struct sockaddr *)&socka, &sl);
    if (latcp_client_fd >= 0)
    {
	debuglog(("Got LATCP connection\n"));
	latcp_circuits[latcp_client_fd] = new LATCPCircuit(latcp_client_fd);

	fdlist.push_back(fdinfo(latcp_client_fd, 0, LATCP_SOCKET));
    }
    else
	syslog(LOG_WARNING, "accept on latcp failed: %m");
}

// Got a new connection from llogin
void LATServer::accept_llogin(int fd)
{
    struct sockaddr_un socka;
    socklen_t sl = sizeof(socka);

    int llogin_client_fd = accept(fd, (struct sockaddr *)&socka, &sl);
    if (llogin_client_fd >= 0)
    {
	debuglog(("Got llogin connection\n"));
	latcp_circuits[llogin_client_fd] = new LLOGINCircuit(llogin_client_fd);

	fdlist.push_back(fdinfo(llogin_client_fd, 0, LLOGIN_SOCKET));
    }
    else
	syslog(LOG_WARNING, "accept on llogin failed: %m");
}

// Read a request from LATCP
void LATServer::read_latcp(int fd)
{
    debuglog(("Got command on latcp socket: %d\n", fd));

    if (!latcp_circuits[fd]->do_command())
    {
	// Mark the FD for removal
	deleted_session s(LATCP_SOCKET, 0, 0, fd);
	dead_session_list.push_back(s);
    }
}

// Read a request from LLOGIN
void LATServer::read_llogin(int fd)
{
    debuglog(("Got command on llogin socket: %d\n", fd));

    if (!latcp_circuits[fd]->do_command())
    {
	// Mark the FD for removal
	deleted_session s(LLOGIN_SOCKET, 0, 0, fd);
	dead_session_list.push_back(s);
    }
}

void LATServer::delete_entry(deleted_session &dsl)
{
    switch (dsl.get_type())
    {
    case INACTIVE:
	break; // do nothing;

    case LAT_SOCKET:
    case LATCP_RENDEZVOUS:
    case LLOGIN_RENDEZVOUS:
	// These never get deleted
	break;

    case LATCP_SOCKET:
    case LLOGIN_SOCKET:
	remove_fd(dsl.get_fd());
	close(dsl.get_fd());
	delete latcp_circuits[dsl.get_fd()];
	latcp_circuits.erase(dsl.get_fd());
	break;

    case LOCAL_PTY:
    case DISABLED_PTY:
	remove_fd(dsl.get_fd());
	if (connections[dsl.get_conn()])
	    connections[dsl.get_conn()]->remove_session(dsl.get_id());
	break;

    default:
       debuglog(("Unknown socket type: %d\n", dsl.get_type()));
       break;
    }
}

void LATServer::process_data(fdinfo &fdi)
{
    switch (fdi.get_type())
    {
    case INACTIVE:
    case DISABLED_PTY:
	break; // do nothing;

    case LOCAL_PTY:
	fdi.get_localport()->do_read();
	break;

    case LAT_SOCKET:
	read_lat(fdi.get_fd());
	break;

    case LATCP_RENDEZVOUS:
	accept_latcp(fdi.get_fd());
	break;

    case LLOGIN_RENDEZVOUS:
	accept_llogin(fdi.get_fd());
	break;

    case LATCP_SOCKET:
	read_latcp(fdi.get_fd());
	break;

    case LLOGIN_SOCKET:
	read_llogin(fdi.get_fd());
	break;
    }
}

// Send a circuit disconnect with error code
void LATServer::send_connect_error(int reason, LAT_Header *msg, int interface,
				   unsigned char *macaddr)
{
    unsigned char buf[1600];
    LAT_Header *header = (LAT_Header *)buf;
    int ptr=sizeof(LAT_Header);

    header->cmd             = LAT_CCMD_DISCON;
    header->num_slots       = 0;
    header->local_connid    = msg->remote_connid;
    header->remote_connid   = msg->local_connid;
    header->sequence_number = msg->sequence_number;
    header->ack_number      = msg->ack_number;
    buf[ptr++]              = reason;
    send_message(buf, ptr, interface, macaddr);
}

// Shutdown by latcp
void LATServer::Shutdown()
{
    // Shutdown all the connections first
    for (int i=1; i<MAX_CONNECTIONS; i++)
    {
	if (connections[i])
	    dead_connection_list.push_back(i);
    }

    // Exit the main loop.
    do_shutdown = true;
}

// Return true if 'name' is a local service
bool LATServer::is_local_service(char *name)
{
    // Look for it.
    std::list<serviceinfo>::iterator sii;
    sii = find(servicelist.begin(), servicelist.end(), name);
    if (sii != servicelist.end()) return true;

    return false;
}

// Return the command info for a service
int LATServer::get_service_info(char *name, std::string &cmd, int &maxcon, int &curcon, uid_t &uid, gid_t &gid)
{
    // Look for it.
    std::list<serviceinfo>::iterator sii;
    sii = find(servicelist.begin(), servicelist.end(), name);
    if (sii != servicelist.end())
    {
	cmd = sii->get_command();
	maxcon = sii->get_max_connections();
	curcon = sii->get_cur_connections();
	uid = sii->get_uid();
	gid = sii->get_gid();

	return 0;
    }

    return -1;
}

// Add a new service from latcp
bool LATServer::add_service(char *name, char *ident, char *command, int max_conn,
			    uid_t uid, gid_t gid, int _rating, bool _static_rating)
{
    // Look for it.
    std::list<serviceinfo>::iterator sii;
    sii = find(servicelist.begin(), servicelist.end(), name);
    if (sii != servicelist.end()) return false; // Already exists

    // if rating is 0 then use the node default.
    if (!_rating) _rating = rating;

    servicelist.push_back(serviceinfo(name,
				      _rating,
				      _static_rating,
				      ident,
				      max_conn,
				      command,
				      uid, gid));

    // Resend the announcement message.
    send_service_announcement(-1);

    return true;
}


// Change the rating of a service
bool LATServer::set_rating(char *name, int _rating, bool _static_rating)
{
    // Look for it.
    std::list<serviceinfo>::iterator sii;
    sii = find(servicelist.begin(), servicelist.end(), name);
    if (sii == servicelist.end()) return false; // Not found it

    sii->set_rating(_rating, _static_rating);

    // Resend the announcement message.
    send_service_announcement(-1);
    return true;
}

// Change the ident of a service
bool LATServer::set_ident(char *name, char *ident)
{
    // Look for it.
    std::list<serviceinfo>::iterator sii;
    sii = find(servicelist.begin(), servicelist.end(), name);
    if (sii == servicelist.end()) return false; // Not found it

    debuglog(("Setting ident for %s to '%s'\n", name, ident));

    sii->set_ident(ident);

    // Resend the announcement message.
    send_service_announcement(-1);
    return true;
}


// Remove reverse-LAT port via latcp
bool LATServer::remove_port(char *name)
{
    debuglog(("remove port %s\n", name));

    // Search for it.
    std::list<LocalPort>::iterator p(portlist.begin());
    for (; p != portlist.end(); p++)
    {
	if (strcmp(p->get_devname().c_str(), name) == 0)
	{
	    p->close_and_delete();
	    portlist.erase(p);
	    return true;
	}

    }
    return false;
}



// Remove service via latcp
bool LATServer::remove_service(char *name)
{
    // Only add the service if it does not already exist.
    std::list<serviceinfo>::iterator sii;
    sii = find(servicelist.begin(), servicelist.end(), name);
    if (sii == servicelist.end()) return false; // Does not exist

    servicelist.erase(sii);

    // This is overkill but it gets rid of the service in the known
    // services table.
    LATServices::Instance()->remove_node(std::string((char*)get_local_node()));

    // Resend the announcement message -- this will re-add our node
    // services back into the known services list.
    send_service_announcement(-1);

    return true;
}

// Change the multicast timer
void LATServer::set_multicast(int newtime)
{
    if (newtime)
    {
	multicast_timer = newtime;
	alarm(newtime);
    }
}

// Change the node name
void LATServer::set_nodename(unsigned char *name)
{
    debuglog(("New node name is %s\n", name));

    // Remove all existing node services
    LATServices::Instance()->remove_node(std::string((char*)get_local_node()));

    // Set the new name
    strcpy((char *)local_name, (char *)name);

    // Resend the announcement message -- this will re-add our node
    // services back into the known services list.
    send_service_announcement(-1);

}

// Start sending service announcements
void LATServer::unlock()
{
    sig_blk_t _block(SIGALRM);
    locked = false;
    alarm_signal(SIGALRM);
}

// Generic make_connection code for llogin & port sessions.
int LATServer::make_connection(int fd, const char *service, const char *rnode, const char *port,
			       const char *localport, const char *password, bool queued)
{
    unsigned char macaddr[6];
    std::string servicenode;
    int this_int;
    char node[255];

    // Take a local copy of the node so we can overwrite it.
    strcpy(node, rnode);

    // If no node was specified then use the highest rated one
    if (node[0] == '\0')
    {
debuglog(("make_connection : no node, use highest\n"));
	if (!LATServices::Instance()->get_highest(std::string((char*)service),
						  servicenode, macaddr,
						  &this_int))
	{
	    debuglog(("Can't find service %s\n", service));
	    return -2; // Never eard of it!
	}
	strcpy((char *)node, servicenode.c_str());
    }
    else
    {
debuglog(("make_connection : node : %s\n", node));
	// Try to find the node
	if (!LATServices::Instance()->get_node(std::string((char*)service),
					       std::string((char*)node), macaddr,
					       &this_int))
	{
	    debuglog(("Can't find node %s in service\n", node, service));

	    return -2;
	}
    }

// Look for a connection that's already in use for this node, unless we
// are queued in which case we need to allocate a new connection for
// initiating the connection and receiving status requests on.
    int connid = -1;
    if (!queued) connid = find_connection_by_node(node);
    if (connid == -1)
    {
	// None: create a new one
	connid = get_next_connection_number();
	connections[connid] = new LATConnection(connid,
						(char *)service,
						(char *)port,
						(char *)localport,
						(char *)node,
						queued,
						false);
    }

    return connid;
}


int LATServer::make_llogin_connection(int fd, const char *service, const char *rnode, const char *port,
				      const char *localport, const char *password, bool queued)
{
    int ret;
    int connid;

    connid = make_connection(fd, service, rnode, port, localport, password, queued);
    if (connid < 0)
	return connid;

    debuglog(("lloginSession for %s has connid %d\n", service, connid));

    ret = connections[connid]->create_llogin_session(fd, service, port, localport, password);

    // Remove LLOGIN socket from the list as it's now been
    // added as a PTY (honest!)
    deleted_session s(LLOGIN_SOCKET, 0, 0, fd);
    dead_session_list.push_back(s);

    return ret;
}


// Called when activity is detected on a LocalPort - we connect it
// to the service.
int LATServer::make_port_connection(int fd, LocalPort *lport,
				    const char *service, const char *rnode,
				    const char *port,
				    const char *localport,
				    const char *password,
				    bool queued)
{
    int ret;
    int connid;
    debuglog(("LATServer::make_port_connection : fd %d, lport '0x%X', service '%s', rnode '%s', port '%s', localport '%s', pwd '%s'\n",
	      fd, lport, service, rnode, port, localport, password));

    connid = make_connection(fd, service, rnode, port, localport, password, queued);
    if (connid < 0)
	return connid;

    debuglog(("localport for %s has connid %d\n", service, connid));

    ret = connections[connid]->create_localport_session(fd, lport, service,
							port, localport, password);

    return ret;
}


// Called from latcp to create a /dev/lat/ port
// Here we simply add it to a lookaside list and wait
// for it to be activated by a user.
int LATServer::create_local_port(unsigned char *service,
				 unsigned char *portname,
				 unsigned char *devname,
				 unsigned char *remnode,
				 bool queued,
				 bool clean,
				 unsigned char *password)
{
    debuglog(("Server::create_local_port: %s\n", devname));

// Don't create it if it already exists.
    std::list<LocalPort>::iterator i(portlist.begin());
    for (; i != portlist.end(); i++)
    {
	if (strcmp(i->get_devname().c_str(), (char *)devname) == 0)
	{
	    return 1; // already in use
	}
    }

    portlist.push_back(LocalPort(service, portname, devname, remnode, queued, clean, password));

// Find the actual port in the list and start it up, this is because
// the STL containers hold actual objects rather then pointers
    std::list<LocalPort>::iterator p(portlist.begin());
    for (; p != portlist.end(); p++)
    {
	if (strcmp(p->get_devname().c_str(), (char *)devname) == 0)
	{
	    p->init_port();
	    break;
	}
    }

    return 0;
}


// Make this as much like VMS LATCP SHOW NODE as possible.
bool LATServer::show_characteristics(bool verbose, std::ostrstream &output)
{
    output <<std::endl;
    output.setf(std::ios::left, std::ios::adjustfield);

    output << "Node Name:  ";
    output.width(15);
    output << get_local_node() << " " << "    LAT Protocol Version:       " << LAT_VERSION << "." << LAT_VERSION_ECO << std::endl;
    output.setf(std::ios::right, std::ios::adjustfield);
    output << "Node State: On " << "                 LATD Version:               " << VERSION << std::endl;
    output << "Node Ident: " << greeting << std::endl;
    output << std::endl;

    output << "Service Responder : " << (responder?"Enabled":"Disabled") << std::endl;
    output << "Interfaces        : ";
    for (int i=0; i<num_interfaces; i++)
    {
	output << iface->ifname(interface_num[i]) << " ";
    }
    output << std::endl << std::endl;

    output << "Circuit Timer (msec): " << std::setw(6) << circuit_timer*10 << "    Keepalive Timer (sec): " << std::setw(6) << keepalive_timer << std::endl;
    output << "Retransmit Limit:     " << std::setw(6) << retransmit_limit << std::endl;
    output << "Multicast Timer (sec):" << std::setw(6) << multicast_timer << std::endl;
    output << std::endl;

    // Show groups
    output << "User Groups:     ";
    print_bitmap(output, true, user_groups);
    output << "Service Groups:  ";
    print_bitmap(output, groups_set, groups);
    output << std::endl;

    // Show services we are accepting for.
    output << "Service Name   Status   Rating  Identification" << std::endl;
    std::list<serviceinfo>::iterator i(servicelist.begin());
    for (; i != servicelist.end(); i++)
    {
	output.width(15);
	output.setf(std::ios::left, std::ios::adjustfield);
	output << i->get_name().c_str();
	output.setf(std::ios::right, std::ios::adjustfield);
	output << "Enabled" << std::setw(6) << i->get_rating() <<
	    (i->get_static()?"    ":" D  ") << i->get_id() << std::endl;
    }

    output << std::endl << "Port                    Service         Node            Remote Port     Queued" << std::endl;

    // Show allocated ports
    std::list<LocalPort>::iterator p(portlist.begin());
    for (; p != portlist.end(); p++)
    {
	p->show_info(verbose, output);
    }

    // NUL-terminate it.
    output << std::endl << ends;

    return true;
}

bool LATServer::show_nodes(bool verbose, std::ostrstream &output)
{
    return LATServices::Instance()->list_dummy_nodes(verbose, output);
}

// Return a number for a new connection
int LATServer::get_next_connection_number()
{
    int i;
    for (i=next_connection; i < MAX_CONNECTIONS; i++)
    {
	if (!connections[i])
	{
	    next_connection = i+1;
	    return i;
	}
    }

// Didn't find a slot here - try from the start
    for (i=1; i < next_connection; i++)
    {
	if (!connections[i])
	{
	    next_connection = i+1;
	    return i;
	}
    }
    return -1;
}

int LATServer::set_servergroups(unsigned char *bitmap)
{
    // Assume all unset if this is the first mention of groups
    if (!groups_set)
    {
	memset(groups, 0, 32);
	user_groups[0] = 1; // But enable group 0
    }
    groups_set = true;

    for (int i=0; i<32; i++)
    {
	groups[i] |= bitmap[i];
    }
    return true;
}

int LATServer::unset_servergroups(unsigned char *bitmap)
{
    // Assume all set if this is the first mention of groups
    if (!groups_set)
    {
	memset(groups, 0xFF, 32);
    }
    groups_set = true;

    for (int i=0; i<32; i++)
    {
	groups[i] ^= bitmap[i];
    }
    return true;
}

int LATServer::set_usergroups(unsigned char *bitmap)
{
    for (int i=0; i<32; i++)
    {
	user_groups[i] |= bitmap[i];
    }
    return true;
}

int LATServer::unset_usergroups(unsigned char *bitmap)
{
    for (int i=0; i<32; i++)
    {
	user_groups[i] ^= bitmap[i];
    }
    return true;
}

// Look for a connection for this node name, if not found then
// return -1
int LATServer::find_connection_by_node(const char *node)
{
    debuglog(("Looking for connection to node %s\n", node));
    for (int i=1; i<MAX_CONNECTIONS; i++)
    {
	if (connections[i] &&
	    connections[i]->node_is(node) &&
	    connections[i]->num_clients() < MAX_CONNECTIONS-1)
	{
	    debuglog(("Reusing connection for node %s\n", node));
	    return i;
	}
    }
    return -1;
}

// Print a groups bitmap
// TODO: print x-y format like we accept in latcp.
void LATServer::print_bitmap(std::ostrstream &output, bool isset, unsigned char *bitmap)
{
    if (!isset)
    {
	output << "0" << std::endl;
	return;
    }

    bool printed = false;

    for (int i=0; i<32; i++) // Show bytes
    {
	unsigned char thebyte = bitmap[i];

	for (int j=0; j<8; j++) // Bits in the byte
	{
	    if (thebyte&1)
	    {
		if (printed) output << ",";
		printed = true;
		output << i*8+j;
	    }
	    thebyte = thebyte>>1;
	}
    }
    output << std::endl;
}

LATServer *LATServer::instance = NULL;
unsigned char LATServer::greeting[255] = {'\0'};
