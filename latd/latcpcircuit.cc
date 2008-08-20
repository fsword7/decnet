/******************************************************************************
    (c) 2000-2003 Christine Caulfield                 christine.caulfield@googlemail.com

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
#include <unistd.h>
#include <string.h>

#include <sstream>
#include <list>
#include <string>
#include <map>
#include <queue>
#include <iterator>

#include "lat.h"
#include "latcp.h"
#include "utils.h"
#include "services.h"
#include "session.h"
#include "localport.h"
#include "connection.h"
#include "circuit.h"
#include "latcpcircuit.h"
#include "server.h"


LATCPCircuit::LATCPCircuit(int _fd):
    Circuit(_fd),
    state(STARTING)
{
}


LATCPCircuit::~LATCPCircuit()
{
}

bool LATCPCircuit::do_command()
{
    unsigned char head[3];
    bool retval = true;

    debuglog(("latcp: do_command on fd %d\n", fd));

    // Get the message header (cmd & length)
    if (read(fd, head, sizeof(head)) != 3)
	return false; // Bad header

    int len = head[1] * 256 + head[2];
    unsigned char *cmdbuf = new unsigned char[len];

    // Read the message buffer
    if (read(fd, cmdbuf, len) != len)
    {
	delete[] cmdbuf;
	return false; // Bad message
    }

    // Have we completed negotiation?
    if (head[0] != LATCP_VERSION &&
	state != RUNNING)
    {
	delete[] cmdbuf;
	return false;
    }

    debuglog(("latcp: do_command %d\n", head[0]));

    // Do the command
    switch (head[0])
    {
    case LATCP_VERSION:
	if (strcmp(VERSION, (char*)cmdbuf) == 0)
	{
	    state = RUNNING; // Versions match
	    send_reply(LATCP_VERSION, VERSION, -1);
	}
	else
	{
	    debuglog(("Connect from invalid latcp version %s\n", cmdbuf));
	    send_reply(LATCP_ERRORMSG, "latcp version does not match latd version " VERSION, -1);
	    retval = false;
	}
	break;

    case LATCP_SHOWSERVICE:
    {
	int verbose = cmdbuf[0];
	std::ostringstream st;

	debuglog(("latcp: show_services(verbose=%d)\n", verbose));

	LATServices::Instance()->list_services(verbose?true:false, st);
	send_reply(LATCP_SHOWSERVICE, st.str().c_str(), (int)st.tellp());
//	st.freeze(false);
    }
    break;

    case LATCP_SHOWCHAR:
    {
	int verbose = cmdbuf[0];
	std::ostringstream st;

	debuglog(("latcp: show_characteristics(verbose=%d)\n", verbose));

	LATServer::Instance()->show_characteristics(verbose?true:false, st);
	send_reply(LATCP_SHOWCHAR, st.str().c_str(), (int)st.tellp());
//	st.freeze(false);
    }
    break;

    case LATCP_SHOWNODES:
    {
	int verbose = cmdbuf[0];
	std::ostringstream st;

	debuglog(("latcp: shownodes\n"));

	LATServer::Instance()->show_nodes(verbose?true:false, st);
	send_reply(LATCP_SHOWNODES, st.str().c_str(), (int)st.tellp());
//	st.freeze(false);
    }
    break;


    case LATCP_SETRESPONDER:
    {
	bool onoff = cmdbuf[0]==0?false:true;
	LATServer::Instance()->SetResponder(onoff);
    }
    break;

    case LATCP_UNLOCK:
    {
	debuglog(("UNLOCK received...off we go\n"));
	LATServer::Instance()->unlock();

    }
    break;

    case LATCP_PURGE:
    {
	debuglog(("Purging known services\n"));
	LATServices::Instance()->purge();

    }
    break;

    case LATCP_SETNODENAME:
    {
	unsigned char name[256];
	int ptr = 0;
	get_string((unsigned char*)cmdbuf, &ptr, name);
	LATServer::Instance()->set_nodename(name);
    }
    break;


    case LATCP_SETSERVERGROUPS:
    {
	LATServer::Instance()->set_servergroups(cmdbuf);
	send_reply(LATCP_ACK, "", -1);
    }
    break;

    case LATCP_UNSETSERVERGROUPS:
    {
	LATServer::Instance()->unset_servergroups(cmdbuf);
	send_reply(LATCP_ACK, "", -1);
    }
    break;


    case LATCP_SETUSERGROUPS:
    {
	LATServer::Instance()->set_usergroups(cmdbuf);
	send_reply(LATCP_ACK, "", -1);
    }
    break;

    case LATCP_UNSETUSERGROUPS:
    {
	LATServer::Instance()->unset_usergroups(cmdbuf);
	send_reply(LATCP_ACK, "", -1);
    }
    break;


    // Change the rating of a service
    case LATCP_SETRATING:
    {
	unsigned char name[255];
	bool static_rating;
	int  rating;
	int  ptr=2;

	static_rating = (bool)cmdbuf[0];
	rating = cmdbuf[1];
	get_string((unsigned char*)cmdbuf, &ptr, name);

	if (LATServer::Instance()->set_rating((char*)name, rating, static_rating))
	    send_reply(LATCP_ACK, "", -1);
	else
	    send_reply(LATCP_ERRORMSG, "Local service does not exist", -1);
    }
    break;

    // Change the ident of a service
    case LATCP_SETIDENT:
    {
	unsigned char name[255];
	unsigned char ident[255];
	int  ptr=0;

	get_string((unsigned char*)cmdbuf, &ptr, name);
	get_string((unsigned char*)cmdbuf, &ptr, ident);

	if (LATServer::Instance()->set_ident((char*)name, (char*)ident))
	    send_reply(LATCP_ACK, "", -1);
	else
	    send_reply(LATCP_ERRORMSG, "Local service does not exist", -1);
    }
    break;



    // Add a login service
    case LATCP_ADDSERVICE:
    {
	unsigned char name[255];
	unsigned char ident[255];
	unsigned char command[1024];
	bool static_rating;
	int  rating;
	int  ptr=2;
	int  max_connections;
	uid_t target_uid;
	gid_t target_gid;

	static_rating = (bool)cmdbuf[0];
	rating = cmdbuf[1];
	get_string((unsigned char*)cmdbuf, &ptr, name);
	get_string((unsigned char*)cmdbuf, &ptr, ident);
	max_connections = cmdbuf[ptr++];
	memcpy(&target_uid, cmdbuf+ptr, sizeof(uid_t)); ptr += sizeof(uid_t);
	memcpy(&target_gid, cmdbuf+ptr, sizeof(uid_t)); ptr += sizeof(gid_t);
	get_string((unsigned char*)cmdbuf, &ptr, command);

	debuglog(("latcp: add service: %s (%s)\n",
		  name, ident));

	if (LATServer::Instance()->add_service((char*)name, (char*)ident, (char*)command,
					       max_connections, target_uid, target_gid,
					       rating, static_rating))
	    send_reply(LATCP_ACK, "", -1);
	else
	    send_reply(LATCP_ERRORMSG, "Local service already exists", -1);
    }
    break;

    // Delete service
    case LATCP_REMSERVICE:
    {
	unsigned char name[255];
	int  ptr=0;

	get_string((unsigned char*)cmdbuf, &ptr, name);

	debuglog(("latcp: del service: %s\n", name));

	if (LATServer::Instance()->remove_service((char*)name))
	    send_reply(LATCP_ACK, "", -1);
	else
	    send_reply(LATCP_ERRORMSG, "Local service does not exist", -1);
    }
    break;

    case LATCP_REMPORT:
    {
	unsigned char name[255];
	int  ptr=0;

	get_string((unsigned char*)cmdbuf, &ptr, name);

	debuglog(("latcp: del port: %s\n", name));

	if (LATServer::Instance()->remove_port((char*)name))
	    send_reply(LATCP_ACK, "", -1);
	else
	    send_reply(LATCP_ERRORMSG, "Local port does not exist", -1);
    }
    break;

    // Set the multicast timer
    case LATCP_SETMULTICAST:
    {
	int newtimer;

	memcpy(&newtimer, cmdbuf, sizeof(int));

	debuglog(("latcp: Set multicast: %d\n", newtimer));

	LATServer::Instance()->set_multicast(newtimer);
	send_reply(LATCP_ACK, "", -1);
    }
    break;

    // Set the retransmit limit
    case LATCP_SETRETRANS:
    {
        int newlim;

        memcpy(&newlim, cmdbuf, sizeof(int));

        debuglog(("latcp: Set retransmit limit: %d\n", newlim));

        LATServer::Instance()->set_retransmit_limit(newlim);
        send_reply(LATCP_ACK, "", -1);
    }
    break;

    // Set the keepalive timer
    case LATCP_SETKEEPALIVE:
    {
        int newtimer;

        memcpy(&newtimer, cmdbuf, sizeof(int));

        debuglog(("latcp: Set keepalive timer: %d\n", newtimer));

        LATServer::Instance()->set_keepalive_timer(newtimer);
        send_reply(LATCP_ACK, "", -1);
    }
    break;


    // Connect a port to a remote service
    case LATCP_ADDPORT:
    {
	unsigned char service[255];
	unsigned char remport[255];
	unsigned char localport[255];
	unsigned char remnode[255];
	unsigned char password[255];
	int  ptr=0;
        int  res=0;

	get_string((unsigned char*)cmdbuf, &ptr, service);
	get_string((unsigned char*)cmdbuf, &ptr, remport);
	get_string((unsigned char*)cmdbuf, &ptr, localport);
	get_string((unsigned char*)cmdbuf, &ptr, remnode);
	bool queued = cmdbuf[ptr++];
	bool clean = cmdbuf[ptr++];
	get_string((unsigned char*)cmdbuf, &ptr, password);

	debuglog(("latcp: add port: %s:%s (%s)\n",
		  service, remport, localport));

        //
        // MSC DVK 12-Feb-2002, more verbose error report to user.
        //

	res = LATServer::Instance()->create_local_port(service,
						     remport,
						     localport,
						     remnode,
						     queued,
						     clean,
                                                       password);
        switch (res) {
          case 1:
	    send_reply(LATCP_ERRORMSG, "Local port (tty) already in use", -1);
            break;
          case 0:
	    send_reply(LATCP_ACK, "", -1); // all OK
            break;
	}
    }
    break;


    case LATCP_SHUTDOWN:
    {
	LATServer::Instance()->Shutdown();
    }
    break;

    default:
	retval = false;
	break;
    }

    delete[] cmdbuf;
    return retval;
}

