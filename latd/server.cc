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
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include <ctype.h>
#include <regex.h>
#include <stdlib.h>
#include <utmp.h>
#include <signal.h>
#include <assert.h>
#include <netinet/in.h>
#ifdef HAVE_LIBDNET
#include <netdnet/dn.h>
#include <netdnet/dnetdb.h>
#endif
#include <features.h>    /* for the glibc version number */
#if __GLIBC__ >= 2 && __GLIBC_MINOR >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h>     /* the L2 protocols */
#else
#include <asm/types.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>   /* The L2 protocols */
#endif

#include <list>
#include <queue>
#include <map>
#include <string>
#include <algo.h>
#include <iterator>
#include <string>
#include <strstream>

#include "lat.h"
#include "utils.h"
#include "session.h"
#include "connection.h"
#include "latcpcircuit.h"
#include "server.h"
#include "services.h"
#include "dn_endian.h"

unsigned char *LATServer::get_local_node(void)
{
    static unsigned char local_name[16] = {'\0'};
    unsigned int i;
    
    if (local_name[0] == '\0')
    {
#ifdef HAVE_LIBDNET
	struct dn_naddr *addr;
        addr = getnodeadd();
	sprintf((char *)local_name, "%s", dnet_htoa(addr));
#else
	struct utsname uts;
	uname(&uts);
	if (strchr(uts.nodename, '.'))
	{
	    *strchr(uts.nodename, '.') = '\0';
	}
	strcpy((char *)local_name, uts.nodename);
#endif
	
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
    Instance()->send_service_announcement(sig);

}

/* Called on the multicast timer - advertise our service on the LAN */
void LATServer::send_service_announcement(int sig)
{
    unsigned char packet[1600];
    int ptr;
    struct sockaddr_ll sock_info;
    struct utsname uinfo;
    char  *myname;

    LAT_ServiceAnnounce *announce = (LAT_ServiceAnnounce *)packet;
    ptr = sizeof(LAT_ServiceAnnounce);

    announce->cmd             = LAT_CCMD_SERVICE;
    announce->circuit_timer   = circuit_timer;
    announce->hiver           = 5;    // Highest LAT version acceptable
    announce->lover           = 5;    // Lowest LAT version acceptable
    announce->latver          = 5;    // We do LAT 5.2
    announce->latver_eco      = 2;
    announce->incarnation     = ++multicast_incarnation;
    announce->flags           = 0x6e; // TODO Allow group codes & stuff
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

    announce->group_length    = 1;
    packet[ptr++] = 01; // Groups: TODO support groups
    
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
    list<serviceinfo>::iterator i(servicelist.begin());    
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
	const string name = i->get_name();
	packet[ptr++]     = name.length();
	strcpy((char *)packet+ptr, i->get_name().c_str());
	ptr += name.length();

	// Service Identification
	string id = i->get_id();
	if (id.length() == 0)
	{
	    // Default service identification string
	    char   stringbuf[1024];
	    sprintf(stringbuf, "%s %s", uinfo.sysname, uinfo.release);
	    id = string(stringbuf);
	}

	packet[ptr++] = id.length();
	strcpy((char *)packet+ptr, id.c_str());
	ptr += id.length();

	// Make sure the service table knows about all our services
	if (!do_shutdown)
	    LATServices::Instance()->add_service(string((char*)get_local_node()),
	  				         name,
					         id, real_rating, our_macaddr);
    }
    


    // Not sure what node service classes are
    // probably somthing to do with port services and stuff.
    packet[ptr++] = 0x01; // Node service classes length
    packet[ptr++] = 0x01; // Node service classes

    /* Build the sockaddr_ll structure */
    sock_info.sll_family   = AF_PACKET;
    sock_info.sll_protocol = htons(ETH_P_LAT);
    sock_info.sll_ifindex  = interface_num;
    sock_info.sll_hatype   = 0;
    sock_info.sll_pkttype  = PACKET_MULTICAST;
    sock_info.sll_halen    = 6;

    /* This is the LAT multicast address */
    sock_info.sll_addr[0]  = 0x09;
    sock_info.sll_addr[1]  = 0x00;
    sock_info.sll_addr[2]  = 0x2b;
    sock_info.sll_addr[3]  = 0x00;
    sock_info.sll_addr[4]  = 0x00;
    sock_info.sll_addr[5]  = 0x0f;

    if (sendto(lat_socket, packet, ptr, 0,
	       (struct sockaddr *)&sock_info, sizeof(sock_info)) < 0)
	perror("sendto");

    /* Send it every minute */
    signal(SIGALRM, &alarm_signal);
    alarm(multicast_timer);
}

/* Main loop */
void LATServer::run()
{
    int status;

    // Open LAT protocol socket
    lat_socket = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_LAT));
    if (lat_socket < 0)
    {
	perror("Can't create LAT protocol socket");
	exit(1);
    }
    // Add it to the sockets list    
    fdlist.push_back(fdinfo(lat_socket, 0, LAT_SOCKET));

    // Open LATCP socket
    unlink(LATCP_SOCKNAME);
    latcp_socket = socket(PF_UNIX, SOCK_STREAM, 0);
    if (latcp_socket < 0)
    {
	perror("Can't create latcp socket");
	exit(1);
    }

    struct sockaddr_un sockaddr;
    strcpy(sockaddr.sun_path, LATCP_SOCKNAME);
    sockaddr.sun_family = AF_UNIX;
    if (bind(latcp_socket, (struct sockaddr *)&sockaddr, sizeof(sockaddr)))
    {
        perror("can't bind latcp socket");
        exit(1);
    }
    if (listen(latcp_socket, 1) != 0)
	perror("listen latcp");
    chmod(LATCP_SOCKNAME, 0600);
    fdlist.push_back(fdinfo(latcp_socket, 0, LATCP_RENDEZVOUS));
    
    
    // Start sending service announcements
    alarm_signal(SIGALRM);

    // We rely on Linux's select() behaviour in that we use the
    // time left in the timeval parameter to make sure the circuit timer
    // goes off at a reasonably predictable interval.
    struct timeval tv = {0, circuit_timer*10000};
    do_shutdown = false;
    do
    {
	fd_set fds;    
	FD_ZERO(&fds);

	list<fdinfo>::iterator i(fdlist.begin());
	for (; i != fdlist.end(); i++)
	{
	    if (i->active())
	        FD_SET(i->get_fd(), &fds);
	}

	status = select(FD_SETSIZE, &fds, NULL, NULL, &tv);
	if (status < 0)
	{
	    if (errno != EINTR)
	    {
		perror("Error in select");
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
	      continue;
	    }
	    
	    // Unix will never scale while this is necessary
	    list<fdinfo>::iterator fdl(fdlist.begin());
	    for (; fdl != fdlist.end(); fdl++)
	    {
		if (fdl->get_type() != INACTIVE &&
		    FD_ISSET(fdl->get_fd(), &fds))
		{
		    process_data(*fdl);		    
		}
	    }
	}

	// Tidy deleted sessions
	if (!dead_session_list.empty())
	{
	    list<deleted_session>::iterator dsl(dead_session_list.begin());
	    for (; dsl != dead_session_list.end(); dsl++)
	    {
		delete_entry(*dsl);
	    }
	    dead_session_list.clear();
	}

	// Tidy deleted connections
	if (!dead_connection_list.empty())
	{
	    list<int>::iterator dcl(dead_connection_list.begin());
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

    close(latcp_socket);
    unlink(LATCP_SOCKNAME);

    send_service_announcement(-1); // Say we are unavailable
}

/* LAT socket has something for us */
void LATServer::read_lat(int sock)
{
    unsigned char   buf[1600];
    int    len;
    int    i;
    struct msghdr msg;
    struct iovec iov;
    struct sockaddr_ll sock_info;
    LAT_Header *header = (LAT_Header *)buf;
  
    msg.msg_name = &sock_info;
    msg.msg_namelen = sizeof(sock_info);
    msg.msg_iovlen = 1;
    msg.msg_iov = &iov;
    iov.iov_len = sizeof(buf);
    iov.iov_base = buf;
  
    len = recvmsg(sock, &msg, 0);
    if (len < 0)
    {
	if (errno != EINTR)
	{
	    perror("recvmsg");
	    return;
	}
    }

    // Ignore packets captured in promiscuous mode.
    if (sock_info.sll_pkttype == PACKET_OTHERHOST)
    {
	debuglog(("Got a rogue packet .. interface probably in promiscuous mode\n"));
	return;
    }
    
    // Parse & dispatch it.
    switch(header->cmd)
    {    
    case LAT_CCMD_SREPLY:
    case LAT_CCMD_SDATA:
    case LAT_CCMD_SESSION:
        {
	    debuglog(("session cmd for connid %d\n", header->remote_connid));
	    LATConnection *conn = connections[header->remote_connid];
	    if (conn)
	    {
	        conn->process_session_cmd(buf, len, (unsigned char *)&sock_info.sll_addr);
	    }
	    else
	    {
		// Message format error
		send_connect_error(2, header, (unsigned char *)&sock_info.sll_addr);
	    }
	}
	break;

    case LAT_CCMD_CONNECT:
	// Make a new connection
        // TODO: Check that the connection is really for one of our services
        if ( ((i=make_new_connection(buf, len, header, (unsigned char *)&sock_info.sll_addr) )) > 0)
	{
	    debuglog(("Made new connection: %d\n", i));
	    connections[i]->send_connect_ack();
	}
	break;

    case LAT_CCMD_CONACK:
        {
	    debuglog(("Got connect ACK for %d\n", header->remote_connid));
	    LATConnection *conn = connections[header->remote_connid];
	    if (conn)
	    {
		conn->got_connect_ack(buf);
	    }
	    else
	    {
		// Insufficient resources
		send_connect_error(7, header, (unsigned char *)&sock_info.sll_addr);
	    }
	}
	break;

    case LAT_CCMD_CONREF: // Connection refused
        {
	    debuglog(("Got connect REFUSED for %d\n", header->remote_connid));
	    LATConnection *conn = connections[header->remote_connid];
	    if (conn)
	    {
		delete conn;
		connections[header->remote_connid] = NULL;
	    }
	}

    case LAT_CCMD_DISCON:
        {
	    debuglog(("Disconnecting connection %d: status %x\n", 
		      header->remote_connid,
		      buf[sizeof(LAT_Header)]));
	    if (header->remote_connid <= MAX_CONNECTIONS)
	    {
		LATConnection *conn = connections[header->remote_connid];
		if (conn)
		{
		    delete conn;
		    connections[header->remote_connid] = NULL;
		}
	    }
	    else
	    {
		// Message format error
		send_connect_error(2, header, (unsigned char *)&sock_info.sll_addr);
	    }
	}
	break;

    case LAT_CCMD_SERVICE:
	// Keep a list of known services
	add_services(buf, len, (unsigned char *)&sock_info.sll_addr);
	break;

    case LAT_CCMD_ENQUIRE:
	reply_to_enq(buf, len, (unsigned char *)&sock_info.sll_addr);
	break;
    }
}

// Add an FD to the list of FDs to listen to
void LATServer::add_fd(int fd, fd_type type)
{
    list<fdinfo>::iterator fdi;

    debuglog(("Add_fd: %d\n", fd));

    fdi = find(fdlist.begin(), fdlist.end(), fd);
    if (fdi != fdlist.end()) return; // Already exists


    fdlist.push_back(fdinfo(fdinfo(fd, NULL, type)));
}

// Remove FD from the FD list
void LATServer::remove_fd(int fd)
{
    list<fdinfo>::iterator fdi;
    debuglog(("remove_fd: %d\n", fd));

    fdi = find(fdlist.begin(), fdlist.end(), fd);
    if (fdi == fdlist.end()) return; // Does not exist

    fdlist.remove(*fdi);
}

// Remove FD from the FD list
void LATServer::set_fd_state(int fd, bool disabled)
{
    list<fdinfo>::iterator fdi;
    debuglog(("set_fd_state: %d, %d\n", fd, disabled));

    fdi = find(fdlist.begin(), fdlist.end(), fd);
    if (fdi == fdlist.end()) return; // Does not exist

    fdi->set_disabled(disabled);
}



/* Send a LAT message to a specified MAC address */
int LATServer::send_message(unsigned char *buf, int len, unsigned char *macaddr)
{
  struct sockaddr_ll sock_info;

  if (len < 46) len = 46; // Minimum packet length
  if (len%2) len++;       // Must be an even number 
  
  /* Build the sockaddr_ll structure */
  sock_info.sll_family   = AF_PACKET;
  sock_info.sll_protocol = htons(ETH_P_LAT);
  sock_info.sll_ifindex  = interface_num;
  sock_info.sll_hatype   = 0;
  sock_info.sll_pkttype  = PACKET_MULTICAST;
  sock_info.sll_halen    = 6;  
  memcpy(sock_info.sll_addr, macaddr, 6);
  
  if (sendto(lat_socket, buf, len, 0,
	     (struct sockaddr *)&sock_info, sizeof(sock_info)) < 0)
  {
      perror("sendto");
      return -1;
  }
  return 0;
  
}

/* Get the system load average */
float LATServer::get_loadavg(void)
{
    float a,b,c;
    FILE *f = fopen("/proc/loadavg", "r");
    
    if (!f)
	return 0;

    if (fscanf(f, "%g %g %g", &a, &b, &c) != 3)
	a = b = c = 0.0;

    fclose(f);
    
    return b;
}

/* Reply to an ENQUIRE message with our MAC address & service name */
void LATServer::reply_to_enq(unsigned char *inbuf, int len, 
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
	list<serviceinfo>::iterator sii;
	sii = find(servicelist.begin(), servicelist.end(), (char *)req_service);
	if (sii == servicelist.end()) return; // Not ours
    }
    
    string node;
    if (LATServices::Instance()->get_highest(string((char *)req_service), 
					     node, reply_macaddr))
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

    send_message(outbuf, outptr, remote_mac);
}

void LATServer::shutdown()
{
    do_shutdown = true;
}

void LATServer::init(bool _static_rating, int _rating,
		     char *_service, char *_greeting, int _interface_num,
		     int _verbosity, int _timer, char *_our_macaddr)
{
    // Add the default session
    servicelist.push_back(serviceinfo(_service, 
				      _rating,
				      _static_rating));

    strcpy((char *)greeting, _greeting);
    interface_num = _interface_num;
    verbosity = _verbosity;

// Save these two for any newly added services
    rating = _rating;
    static_rating = _static_rating;
    
    memcpy(our_macaddr, _our_macaddr, 6);
    next_connection = 1;
    multicast_incarnation = 0;
    circuit_timer = _timer/10;

    memset(connections, 0, sizeof(connections));
}

// Create a new connection object for this remote node.
int LATServer::make_new_connection(unsigned char *buf, int len, 
				   LAT_Header *header,
				   unsigned char *macaddr)
{
    int i;
    i = get_next_connection_number();

    if (i >= 0)
    {
	next_connection = i+1;
	connections[i] = new LATConnection(i, buf, len,
					   header->sequence_number,
					   header->ack_number,
					   macaddr);
    }
    else
    {
// Number of virtual circuits exceeded
	send_connect_error(9, header, macaddr); 
	return -1;
    }
    return i;
}

void LATServer::delete_session(LATConnection *conn, unsigned char id, int fd)
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

// Add services received from a service announcement multicast
void LATServer::add_services(unsigned char *buf, int len, unsigned char *macaddr)
{
    LAT_ServiceAnnounce *announce = (LAT_ServiceAnnounce *)buf;
    int ptr = sizeof(LAT_ServiceAnnounce);
    unsigned char nodename[16];
    unsigned char greeting[255];
    unsigned char service[255];
    unsigned char ident[255];

    // Skip groups
    // TODO Honour group numbers(?)
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
	    LATServices::Instance()->add_service(string((char*)nodename), 
						 string((char*)service),
						 string((char*)ident), rating, macaddr);
	}
	else
	{
	    LATServices::Instance()->remove_node(string((char*)nodename));
	}
    }
}

// Wait for data available on a client PTY
void LATServer::add_pty(LATSession *session, int fd)
{
    fdlist.push_back(fdinfo(fd, session, LOCAL_PTY));
}


// Got a new connection from latcp
void LATServer::accept_latcp(int fd)
{
    struct sockaddr_un socka;
    socklen_t sl;
    
    int latcp_client_fd = accept(fd, (struct sockaddr *)&socka, &sl);
    if (latcp_client_fd >= 0)
    {
	debuglog(("Got LATCP connection\n"));
	latcp_circuits[latcp_client_fd] = LATCPCircuit(latcp_client_fd);
	
	fdlist.push_back(fdinfo(latcp_client_fd, 0, LATCP_SOCKET));	
    }
    else
	perror("accept");
}

// Read a request from LATCP
void LATServer::read_latcp(int fd)
{
    debuglog(("Got command on latcp socket: %d\n", fd));
    
    if (!latcp_circuits[fd].do_command())
    {
	// Mark the FD for removal
	deleted_session s(LATCP_SOCKET, NULL, 0, fd);
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
	// These two never get deleted
	break;
	
    case LATCP_SOCKET:
	remove_fd(dsl.get_fd());
	close(dsl.get_fd());
	latcp_circuits.erase(dsl.get_fd());
	break;
	
    case LOCAL_PTY:
    case DISABLED_PTY:
	remove_fd(dsl.get_fd());	
	dsl.get_conn()->remove_session(dsl.get_id());
	break;


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
	fdi.get_session()->do_read();
	break;

    case LAT_SOCKET:
	read_lat(fdi.get_fd());
	break;
	
    case LATCP_RENDEZVOUS:
	accept_latcp(fdi.get_fd());
	break;
	
    case LATCP_SOCKET:
	read_latcp(fdi.get_fd());
	break;
    }
}

// Send a circuit disconnect with error code
void LATServer::send_connect_error(int reason, LAT_Header *msg, unsigned char *macaddr)
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
    send_message(buf, ptr, macaddr);
}

// Shutdown bu latcp
void LATServer::Shutdown()
{
    // Shutdown all the connections first
    for (int i=1; i<MAX_CONNECTIONS; i++)
    {
	// TODO: Send any shutdown messages first??
	if (connections[i])
	    dead_connection_list.push_back(i);
    }

    // Exit the main loop.
    do_shutdown = true;
}

// Add a new service from latcp
void LATServer::add_service(char *name, char *ident)
{
    // Look for it.
    list<serviceinfo>::iterator sii;
    sii = find(servicelist.begin(), servicelist.end(), name);
    if (sii != servicelist.end()) return; // Already exists
    
    servicelist.push_back(serviceinfo(name, 
				      rating,
				      static_rating, 
				      ident));

    // Resend the announcement message.
    send_service_announcement(-1);
}

// Remove service via latcp
void LATServer::remove_service(char *name)
{
    // Only add the service if it does not already exist.
    list<serviceinfo>::iterator sii;
    sii = find(servicelist.begin(), servicelist.end(), name);
    if (sii == servicelist.end()) return; // Does not exist
    
    servicelist.erase(sii);

    // This is ovefkill but it gets rid of the service in the known
    // services table.
    LATServices::Instance()->remove_node(string((char*)get_local_node()));
    
    // Resend the announcement message -- this will re-add our node
    // services back into the known services list.
    send_service_announcement(-1);
}


// Create a new client connection
int LATServer::make_client_connection(unsigned char *service, 
				      unsigned char *portname,
				      unsigned char *devname,
				      unsigned char *remnode, bool queued)
{
    int connid = get_next_connection_number();
    if (connid == -1)
    {
	return -1; // Failed
    }
    // Look up the service name.
    // TODO what to do with the port name????

    unsigned char macaddr[6];
    string node;
    if (!LATServices::Instance()->get_highest(string((char*)service), 
					      node, macaddr))
    {
	debuglog(("Can't find service %s, checking for node %s\n", 
		  service, remnode));
	// Can't find service: look up by node name
	if (!LATServices::Instance()->get_highest(string((char*)remnode), 
						  node, macaddr))
	{
	    debuglog(("Can't find node %s\n", remnode));
	    return -2; // Never eard of it!
	}
    }
    // TODO: what to do with queued???
    connections[connid] = new LATConnection(connid, 
					    (char *)remnode, 
					    (char *)macaddr,
					    (char *)devname);
    connections[connid]->create_client_session();
    return 0;
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


LATServer *LATServer::instance = NULL;
