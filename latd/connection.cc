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

#include <stdio.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <list>
#include <map>
#include <queue>
#include <string>
#include <strstream>
#include <iterator>

#include "lat.h"
#include "utils.h"
#include "session.h"
#include "serversession.h"
#include "clientsession.h"
#include "connection.h"
#include "latcpcircuit.h"
#include "server.h"
#include "services.h"
#include "dn_endian.h"


// Create a server connection
LATConnection::LATConnection(int _num, unsigned char *buf, int len,
			     unsigned char _clcount,
			     unsigned char _svcount,
			     unsigned char *_macaddr):
    num(_num),
    last_sequence_number(_svcount),
    last_ack_number(_clcount),
    role(SERVER)
{
    memcpy(macaddr, (char *)_macaddr, 6);
    int  ptr = sizeof(LAT_Start);
    LAT_Start *msg = (LAT_Start *)buf;

    debuglog(("New connection: (c: %x, s: %x)\n",
             last_sequence_number, last_ack_number));
    
    get_string(buf, &ptr, servicename);
    get_string(buf, &ptr, remnode);

    debuglog(("Connect from %s (LAT %d.%d) for %s, window size: %d\n",
	     remnode, msg->latver, msg->latver_eco, servicename, msg->exqueued));

    remote_connid = msg->header.local_connid;
    next_session = 1;
    max_window_size = msg->exqueued+1;
    window_size = 0;
    lat_eco = msg->latver_eco;

    // I don't think this is actually true (TODO CHECK!)
    // but it seems to work.
    if (max_window_size < 4)
	max_slots_per_packet = 1;
    else
	max_slots_per_packet = 4;
    
    memset(sessions, 0, sizeof(sessions));
}

// Create a client connection
LATConnection::LATConnection(int _num, const char *_remnode,
			     const char *_macaddr, const char *_lta):
    num(_num),
    last_sequence_number(0xff),
    last_ack_number(0xff),
    role(CLIENT)
{
    debuglog(("New client connection for %s created\n", remnode));
    memcpy(macaddr, _macaddr, 6);
    memset(sessions, 0, sizeof(sessions));
    strcpy((char *)remnode, _remnode);
    strcpy(lta_name, _lta);

    max_slots_per_packet = 1; // TODO: Calculate
    max_window_size = 1;      // Gets overridden later on.
    next_session = 1;

}


bool LATConnection::process_session_cmd(unsigned char *buf, int len, 
					unsigned char *macaddr)
{
    int  msglen;
    unsigned int command;
    int  i;
    int  newsessionnum;
    LATSession *newsession;
    int  ptr = sizeof(LAT_Header);
    int  replylen   = 0;
    bool replyhere  = false;
    int  replyslots = 1;
    unsigned char retcmd = 0;
    LAT_SessionCmd *msg = (LAT_SessionCmd *)buf;
    
    debuglog(("process_session_cmd: %d slots, %d bytes\n",
             msg->header.num_slots, len));

    window_size--;
    if (window_size < 0) window_size=0;

    // For duplicate checking
    unsigned char real_last_sequence_number = last_sequence_number;
    unsigned char real_last_message_acked   = last_message_acked;

    last_sequence_number = msg->header.ack_number;
    last_message_acked   = msg->header.sequence_number;
  
    debuglog(("     seq: %d,      ack: %d\n", msg->header.sequence_number, msg->header.ack_number));
    debuglog(("last seq: %d, last ack: %d\n", last_sequence_number, last_message_acked));
    debuglog(("last seq: %d, last ack: %d\n", last_message_seq, last_ack_number));

    // Is this a duplicate
    if (real_last_sequence_number == last_sequence_number &&
	real_last_message_acked == last_message_acked)
    {
	debuglog(("Duplicate packet received...ignoring it\n"));
	return false;
    }

    // PJC:TODO: not sure about this. 
    // It doesn't affect the server but it seems to help the client
    last_ack_number = last_message_acked;

    // No blocks? just ACK it (if we're a server)
    if (msg->header.num_slots == 0)
    {
	if (role == SERVER) replyhere=true;
	replyslots=0;

	LAT_SlotCmd *slotcmd = (LAT_SlotCmd *)(buf+ptr);
	unsigned char credits = slotcmd->cmd & 0x0F;
	
	debuglog(("No data: cmd: %d, credit: %d\n", msg->header.cmd, credits));
	
	if (credits)
	{	    
	    LATSession *session = sessions[slotcmd->local_session];
	    if (session) session->add_credit(credits);
	}       
    }
    else
    {
        for (i=0; i<msg->header.num_slots && ptr<len; i++)
	{
	    LAT_SlotCmd *slotcmd = (LAT_SlotCmd *)(buf+ptr);
	    msglen  = slotcmd->length;
	    command = slotcmd->cmd & 0xF0;
	    unsigned char credits = slotcmd->cmd & 0x0F;

	    debuglog(("process_slot_cmd(%d:%x). command: %x, credit: %d, len: %d\n",
		     i, ptr, command, credits, msglen));
	    	    
	    ptr += sizeof(LAT_SlotCmd);

	    // Process the data.
	    LATSession *session = sessions[slotcmd->local_session];
	    if (session && credits) session->add_credit(credits);

	    switch (command)
	    {
	    case 0x00:
            {
                if (session)
                {
                    if (session->send_data_to_process(buf+ptr, msglen))
                    {
                        // No echo.
                        replyhere = true;
                    }
                    // We are expecting an echo - don't send anything now
		    // but still increment the remote credit if the other end
		    // has run out.
		    if (session->get_remote_credit() < 1)
		    {
			retcmd |= 15;
			session->inc_remote_credit(15);
		    }
                }
                else
                {
                    // An error - send a disconnect.
		    replyhere  = true;
		    replyslots = 1;
		    retcmd = 0xd3; // Invalid slot recvd
                }
            }
            break;
	  
	    case 0x90:
		if (role == SERVER)
		{
		    newsessionnum = next_session_number();
		    newsession = new ServerSession(*this,
						   (LAT_SessionStartCmd *)buf,
						   slotcmd->remote_session, 
						   newsessionnum);
		    if (newsession->new_session(remnode, 
						credits) == -1)
		    {
			newsession->send_disabled_message();
			delete newsession;
		    }
		    else
		    {
			sessions[newsessionnum] = newsession;
		    }
		}
		else // CLIENT
		{
		    if (session)
			((ClientSession *)session)->got_connection(
			    slotcmd->remote_session);
		}    
		break;

	    case 0xa0:
		// Data_b message - port information
		if (session)
		    session->set_port((unsigned char *)slotcmd);
		replyhere=true;
		break;
	  
	    case 0xb0:
                // Attention. Force XON, Abort etc.
	        break;
		

	    case 0xc0: // Don't know what this is
		replyhere = true;

	    case 0xd0:  // Disconnect
		// TODO: something not quite right here.
		if (session) session->disconnect_session();
		retcmd = 0xd0;
		replyhere=true;
		break;	  

	    default:
		debuglog(("Unknown slot command %x found. length: %d\n",
                         command, msglen));
		replyhere=true;
		break;
	    }
	    ptr += msglen;
	    if (ptr%2) ptr++; // Word-aligned
        }
    }

    // ACK the message if we did nothing else
    if (replyhere)
    {
	unsigned char replybuf[1600];
	LAT_SessionReply *reply  = (LAT_SessionReply *)replybuf;
	
	reply->header.cmd          = LAT_CCMD_SREPLY;
	reply->header.num_slots    = replyslots;
	reply->slot.remote_session = msg->slot.local_session;
	reply->slot.local_session  = msg->slot.remote_session;
	reply->slot.length         = replylen;
	reply->slot.cmd            = retcmd;

	if (role == CLIENT) reply->header.cmd |= 2; // To Host

	ptr = sizeof(LAT_SessionReply);
	memcpy(replybuf+ptr, replybuf, replylen);

	send_message(replybuf, ptr, REPLY);
	return true;
    }
    
    return false;
}


void LATConnection::send_connect_ack()
{
    unsigned char reply[1600];
    int ptr;
    LAT_StartResponse *response = (LAT_StartResponse *)reply;
    LATServer *server = LATServer::Instance();

    // Send response...
    response->header.cmd       = LAT_CCMD_CONACK;
    response->header.num_slots = 0;
    response->maxsize          = dn_htons(1500);
    response->latver           = 5;   // We do LAT 5.2
    response->latver_eco       = 2;    
    response->maxsessions      = 254;
    response->exqueued         = 0;   // TODO Make 9 when we know how to cope.
    response->circtimer        = 8;
    response->keepalive        = 20;  // seconds
    response->facility         = dn_htons(0);
    response->prodtype         = 3;   // Wot do we use here???
    response->prodver          = 3;   // and here ???

    ptr = sizeof(LAT_StartResponse);
    add_string(reply, &ptr, server->get_local_node());
    add_string(reply, &ptr, remnode);
    add_string(reply, &ptr, (unsigned char*)"LAT for Linux");
    reply[ptr++] = '\0';
    
    send_message(reply, ptr, DATA);
}

// Send a message on this connection NOW
int LATConnection::send_message(unsigned char *buf, int len, send_type type)
{
    LAT_Header *response = (LAT_Header *)buf;
    
    if (type == DATA)  
    {
	last_sequence_number++;
	need_ack = true;
	retransmit_count = 0;
	last_message_seq = last_sequence_number;
	window_size++;
    }

    if (type == REPLY) 
    {
	last_ack_number++;
	need_ack = false;
    }

    response->local_connid    = num;
    response->remote_connid   = remote_connid;
    response->sequence_number = last_sequence_number;
    response->ack_number      = last_ack_number;

    debuglog(("Sending message for connid %d\n", num));

    if (need_ack) last_message = pending_msg(buf, len);    
    return LATServer::Instance()->send_message(buf, len, macaddr);
}


// Send a message on this connection when we can
int LATConnection::queue_message(unsigned char *buf, int len)
{
    LAT_Header *response = (LAT_Header *)buf;
    
    response->local_connid    = num;
    response->remote_connid   = remote_connid;
    response->sequence_number = last_sequence_number;
    response->ack_number      = last_ack_number;

    debuglog(("Queued messsge for connid %d\n", num));
    
    pending.push(pending_msg(buf, len));
    return 0;
}


// Enque a slot message on this connection
void LATConnection::send_slot_message(unsigned char *buf, int len)
{
    slots_pending.push(slot_cmd(buf,len));
}

LATConnection::~LATConnection()
{
    debuglog(("LATConnection dtor: %d\n", num));

    // Delete all sessions
    for (unsigned int i=1; i<MAX_SESSIONS; i++)
    {
        if (sessions[i])
        {
            delete sessions[i];
            sessions[i] = NULL;
        }
    }
// Send a "shutting down" message to the other end
    
}

// Generate the next session ID
int LATConnection::next_session_number()
{
    unsigned int i;
    for (i=next_session; i < MAX_SESSIONS; i++)
    {
	if (!sessions[i])
	{
	    next_session = i+1;
	    return i;
	}
    }

// Didn't find a slot here - try from the start 
    for (i=1; i < next_session; i++)
    {
	if (!sessions[i])
	{
	    next_session = i+1;
	    return i;
	}
    }
    return -1;
}


//
//  Send queued packets
//
void LATConnection::circuit_timer(void)
{
    // Did we get an ACK for our last message?
    if (need_ack && last_message_acked != last_message_seq)
    {
	if (++retransmit_count > LATServer::Instance()->get_retransmit_limit())
	{
	    debuglog(("hit retransmit limit on connection %d\n", num));
	    need_ack = false;

	    unsigned char buf[1600];
	    LAT_Header *header = (LAT_Header *)buf;
	    int ptr=sizeof(LAT_Header);
	    
	    header->cmd             = LAT_CCMD_DISCON;
	    header->num_slots       = 0;
	    header->local_connid    = num;
	    header->remote_connid   = remote_connid;
	    header->sequence_number = last_sequence_number;
	    header->ack_number      = last_ack_number;
	    buf[ptr++] = 0x06; // Retransmission limit reached.
	    LATServer::Instance()->send_message(buf, ptr, macaddr);

	    LATServer::Instance()->delete_connection(num);
	    
	    // Mark this node as unavailable in the service list
	    LATServices::Instance()->remove_node(string((char *)remnode));
	    return;
	}	
	debuglog(("Last message not ACKed: RESEND\n"));
	last_message.send(macaddr);
    }
    else
    {
	retransmit_count = 0;
	need_ack = false;
    }

    // Poll our sessions
    for (unsigned int i=0; i<MAX_SESSIONS; i++)
    {
	if (sessions[i])
	    sessions[i]->read_pty();
    }

    
    // Coalesce pending messages and queue them
    while (!slots_pending.empty())
    {
	debuglog(("circuit Timer:: slots pending = %d\n", slots_pending.size()));
        unsigned char buf[1600];
        LAT_Header *header = (LAT_Header *)buf;
        int len=sizeof(LAT_Header);
	if (role == SERVER)
	    header->cmd         = LAT_CCMD_SDATA;
	else
	    header->cmd         = LAT_CCMD_SESSION;
        header->num_slots       = 0;
        header->local_connid    = num;
        header->remote_connid   = remote_connid;
        header->sequence_number = last_sequence_number;
        header->ack_number      = last_ack_number;

	// Send as many slot data messages as we can
	while ( (header->num_slots < max_slots_per_packet && !slots_pending.empty()))
        {
            header->num_slots++;
          
            slot_cmd &cmd(slots_pending.front());

            memcpy(buf+len, cmd.get_buf(), cmd.get_len());
            len += cmd.get_len();
            if (len%2) len++;// Keep it on even boundary
          
            slots_pending.pop();
        }
	if (header->num_slots)
	{
	    debuglog(("Sending %d slots on circuit timer\n", header->num_slots));
	    pending.push(pending_msg(buf, len));
	}
    }

#ifdef VERBOSE_DEBUG
    if (!pending.empty())
    {
      debuglog(("Window size: %d, max %d\n", window_size, max_window_size));
    }
#endif

    //  Send a pending message (if we can)
    if (!pending.empty() && window_size < max_window_size)
    {
        // Send the top message
        pending_msg &msg(pending.front());
      
        last_sequence_number++;
        last_message_seq = last_sequence_number;
        need_ack = true;
	retransmit_count = 0;
	
        LAT_Header *header = msg.get_header();
        header->sequence_number = last_sequence_number;
        header->ack_number      = last_ack_number;
     
        debuglog(("Sending message on circuit timer: seq: %d, ack: %d\n",
		  last_sequence_number, last_ack_number));
 
        msg.send(macaddr);
	last_message = msg; // Save it in case it gets lost on the wire;
        pending.pop();
	window_size++;
    }
}


void LATConnection::remove_session(unsigned char id)
{
    debuglog(("Deleting session %d\n", id));
    if (sessions[id])
    {
        delete sessions[id];
	sessions[id] = NULL;
    }
}

int LATConnection::connect()
{
    int ptr;
    unsigned char buf[1600];
    LAT_Start *msg = (LAT_Start *)buf;
    ptr = sizeof(LAT_Start);

    msg->header.cmd          = LAT_CCMD_CONNECT;
    msg->header.num_slots    = 0;
    msg->header.local_connid = num;  

    msg->maxsize     = dn_htons(1500);
    msg->latver      = 5;   // We do LAT 5.2
    msg->latver_eco  = 2;   // Pretty arbitrary really.
    msg->maxsessions = 16;  // Probably ought to be 254
    msg->exqueued    = 0;   // TODO: A decision here
    msg->circtimer   = 8;   // TODO: Get from LATServer()
    msg->keepalive   = 20;  // seconds
    msg->facility    = dn_htons(0); // Eh?
    msg->prodtype    = 3;   // Wot do we use here???
    msg->prodver     = 3;   // and here ???

    add_string(buf, &ptr, remnode);
    add_string(buf, &ptr, LATServer::Instance()->get_local_node());

    return send_message(buf, ptr, LATConnection::DATA);
}

int LATConnection::create_client_session()
{
// Create a ClientSession

    int newsessionnum = next_session_number();
    LATSession *newsession = new ClientSession(*this, 0,
					       newsessionnum, lta_name);
    if (newsession->new_session(remnode, 0) == -1)
    {
	delete newsession;
	return -1;
    }
    sessions[newsessionnum] = newsession;
    return 0;
}

int LATConnection::got_connect_ack(unsigned char *buf)
{
    LAT_StartResponse *reply = (LAT_StartResponse *)buf;
    remote_connid = reply->header.local_connid;

    last_sequence_number = reply->header.ack_number;
    last_message_acked   = reply->header.sequence_number;
    max_window_size = reply->exqueued+1;

    last_ack_number = last_message_acked;



// Start clientsession 1
    ClientSession *cs = (ClientSession *)sessions[1];
    if (cs) cs->connect();
    else
    {
	// TODO Disconnect
    }

    return 0;
}


int LATConnection::pending_msg::send(unsigned char *macaddr)
{
    return LATServer::Instance()->send_message(buf, len, macaddr);
}
