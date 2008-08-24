/******************************************************************************
    (c) 2000-2005 Christine Caulfield                 christine.caulfield@googlemail.com

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
#include <string.h>
#include <list>
#include <map>
#include <queue>
#include <string>
#include <sstream>
#include <iterator>


#include "lat.h"
#include "utils.h"
#include "session.h"
#include "serversession.h"
#include "reversesession.h"
#include "clientsession.h"
#include "lloginsession.h"
#include "localportsession.h"
#include "queuedsession.h"
#include "localport.h"
#include "connection.h"
#include "circuit.h"
#include "latcpcircuit.h"
#include "server.h"
#include "services.h"
#include "lat_messages.h"
#include "dn_endian.h"


// Create a server connection
LATConnection::LATConnection(int _num, unsigned char *buf, int len,
			     int _interface,
			     unsigned char _seq,
			     unsigned char _ack,
			     unsigned char *_macaddr):
    num(_num),
    interface(_interface),
    keepalive_timer(0),
    last_sent_seq(0xff),
    last_sent_ack(0),
    last_recv_seq(_seq),
    last_recv_ack(_ack),
    last_time(0L),
    need_ack(false),
    queued(false),
    eightbitclean(false),
    connected(false),
    delete_pending(false),
    request_id(0),
    last_msg_type(0),
    last_msg_retries(0),
    role(SERVER),
    send_ack(false)
{
    memcpy(macaddr, (char *)_macaddr, 6);
    int  ptr = sizeof(LAT_Start);
    LAT_Start *msg = (LAT_Start *)buf;

    debuglog(("New connection: (s: %x, a: %x)\n",
             last_recv_seq, last_recv_ack));

    get_string(buf, &ptr, servicename); // This is actually local nodename
    get_string(buf, &ptr, remnode);

    debuglog(("Connect from %s (LAT %d.%d) for %s, window size: %d\n",
	     remnode, msg->latver, msg->latver_eco, servicename, msg->exqueued));

    remote_connid = msg->header.local_connid;
    next_session = 1;
    highest_session = 1;
    max_window_size = msg->exqueued+1;
    max_window_size = 1; // All we can manage
    window_size = 0;
    lat_eco = msg->latver_eco;
    max_slots_per_packet = 4;

    memset(sessions, 0, sizeof(sessions));
}

// Create a client connection
LATConnection::LATConnection(int _num, const char *_service,
			     const char *_portname, const char *_lta,
			     const char *_remnode, bool queued, bool clean):
    num(_num),
    keepalive_timer(0),
    last_sent_seq(0xff),
    last_sent_ack(0),
    last_recv_seq(0),
    last_recv_ack(0xff),
    last_time(0L),
    need_ack(false),
    queued(queued),
    eightbitclean(clean),
    connected(false),
    connecting(false),
    delete_pending(false),
    request_id(0),
    last_msg_type(0),
    last_msg_retries(0),
    role(CLIENT),
    send_ack(false)
{
    debuglog(("New client connection for %s created\n", _remnode));
    memset(sessions, 0, sizeof(sessions));
    strcpy((char *)servicename, _service);
    strcpy((char *)portname, _portname);
    strcpy((char *)remnode, _remnode);
    strcpy(lta_name, _lta);

    max_slots_per_packet = 4;
    max_window_size = 1;      // Gets overridden later on.
    window_size = 0;
    next_session = 1;
    highest_session = 1;
}


bool LATConnection::process_session_cmd(unsigned char *buf, int len,
					unsigned char *macaddr)
{
    int  msglen;
    unsigned int command;
    int  i;
    int  newsessionnum;
    int  ptr = sizeof(LAT_Header);
    LATSession *newsession;
    LAT_SessionCmd *msg = (LAT_SessionCmd *)buf;
    int num_replies = 0;
    LAT_SlotCmd *reply[4];
    char replybuf[4][256];
    bool replyhere = false;

    debuglog(("process_session_cmd: %d slots, %d bytes\n",
             msg->header.num_slots, len));

    /* Clear out the reply slots and initialise pointers */
    memset(replybuf, 0, sizeof(replybuf));
    for (int ri=0; ri<4; ri++)
	reply[ri] = (LAT_SlotCmd *)replybuf[ri];

#ifdef REALLY_VERBOSE_DEBUGLOG
    debuglog(("MSG:      seq: %d,        ack: %d (last sent seq = %d)\n",
	      msg->header.sequence_number, msg->header.ack_number, last_sent_seq));

    debuglog(("PREV:last seq: %d,   last ack: %d\n",
	      last_recv_seq, last_recv_ack));
#endif

    // Is this a duplicate?
    // Check the previous ack number too as we could be one packet out
    if (msg->header.sequence_number == last_recv_seq ||
	msg->header.sequence_number == last_recv_seq-1)
    {
	if (msg->header.ack_number == last_recv_ack)
	{
	    debuglog(("Duplicate packet received...resending ACK\n"));

	    // But still send an ACK as it could be the ACK that went missing
	    last_ack_message.send(interface, last_recv_seq, macaddr);

	    // If the last DATA message wasn't seen either then resend that too
	    if (last_message.get_seq() > msg->header.ack_number ||
		(last_message.get_seq() == 0 && msg->header.ack_number == 0xff))

	    {
		debuglog(("Also sending last DATA message (%d)\n", last_message.get_seq()));
		last_message.send(interface,  last_recv_seq, macaddr);
	    }
	}
	return false;
    }

    // If we got an old message then process that.
    if (msg->header.ack_number != last_sent_seq)
    {
	debuglog(("Got ack for old message, resending ACK\n"));
	last_ack_message.send(interface,  last_recv_seq, macaddr);

	// If the last DATA message wasn't seen either then resend that too
	if (last_message.get_seq() > msg->header.ack_number ||
	    (last_message.get_seq() == 0 && msg->header.ack_number == 0xff))
	{
	    debuglog(("Also sending last DATA message (%d)\n", last_message.get_seq()));
	    last_message.send(interface,  last_recv_seq, macaddr);
	}
    }

    window_size--;
    if (window_size < 0) window_size = 0;

    need_ack = false;
    last_recv_ack = msg->header.ack_number;
    last_recv_seq = msg->header.sequence_number;

    // No blocks? just ACK it (if we're a server)
    if (msg->header.num_slots == 0)
    {
	if (role == SERVER)
	{
	    reply[0]->remote_session = msg->slot.local_session;
	    reply[0]->local_session = msg->slot.remote_session;
	    reply[0]->length = 0;
	    reply[0]->cmd = 0;
	    replyhere = true;
	}

	LAT_SlotCmd *slotcmd = (LAT_SlotCmd *)(buf+ptr);
	unsigned char credits = slotcmd->cmd & 0x0F;

	debuglog(("No data: cmd: %d, credit: %d\n", msg->header.cmd, credits));

	LATSession *session = sessions[slotcmd->local_session];
	if (credits)
	{
	    if (session) session->add_credit(credits);
	}
	if (replyhere && session && session->get_remote_credit() <= 2)
	{
	    reply[0]->cmd |= 15; // Add credit
	    session->inc_remote_credit(15);
	}
    }
    else
    {
	replyhere = true;
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
                        if (role == SERVER)
			{
			    replyhere = true;
			}
                    }
                    // We are expecting an echo - don't send anything now
		    // but still increment the remote credit if the other end
		    // has run out.
		    debuglog(("Remote credit is %d\n", session->get_remote_credit()));
		    if (session->get_remote_credit() <= 2)
		    {
			reply[num_replies]->remote_session = slotcmd->local_session;
			reply[num_replies]->local_session = slotcmd->remote_session;
			reply[num_replies]->length = 0;
			reply[num_replies]->cmd = 15; // Just credit
			num_replies++;
			session->inc_remote_credit(15);
		    }
                }
                else
                {
                    // An error - send a disconnect.
		    reply[num_replies]->remote_session = slotcmd->local_session;
		    reply[num_replies]->local_session = slotcmd->remote_session;
		    reply[num_replies]->length = 0;
		    reply[num_replies]->cmd = 0xD3; // Invalid slot recvd
		    num_replies++;
                }
            }
            break;

	    case 0x90:
		if (role == SERVER)
		{
		    int queued_connection;
		    if (is_queued_reconnect(buf, len, &queued_connection))
		    {
			LATConnection **master_conn = LATServer::Instance()->get_connection(queued_connection);
			if (!(*master_conn))
			{
			    debuglog(("Got queued reconnect for non-existant request ID\n"));

			    // Not us mate...
			    reply[num_replies]->remote_session = slotcmd->local_session;
			    reply[num_replies]->local_session = slotcmd->remote_session;
			    reply[num_replies]->length = 0;
			    reply[num_replies]->cmd = 0xD7; // No such service
			    num_replies++;
			}
			else
			{
			    last_msg_type = 0;
			    (*master_conn)->last_msg_type = 0;

			    last_msg_retries= 0;
			    (*master_conn)->last_msg_retries = 0;

			    // Connect a new port session to it
			    ClientSession *cs = (ClientSession *)(*master_conn)->sessions[1];

			    newsessionnum = next_session_number();
			    newsession = new QueuedSession(*this,
							   (LAT_SessionStartCmd *)buf,
							   cs,
							   slotcmd->remote_session,
							   newsessionnum,
							   (*master_conn)->eightbitclean);
			    if (newsession->new_session(remnode, "","",
							credits) == -1)
			    {
				newsession->send_disabled_message();
				delete newsession;
			    }
			    else
			    {
				sessions[newsessionnum] = newsession;
				newsession->set_master_conn(master_conn);

				// If we were pending a delete, we aren't now
				delete_pending = false;
			    }
			}
		    }
		    else
		    {
			//  Check service name is one we recognise.
			ptr = sizeof(LAT_SessionStartCmd);
			unsigned char name[256];
			get_string(buf, &ptr, name);

                        // Do parameters -- look for a 5 (remote port name)
			while (ptr < len)
			{
			    int param_type = buf[ptr++];
			    if (param_type == 5)
			    {
				get_string(buf, &ptr, portname);
			    }
			    else
			    {
				ptr += buf[ptr]+1; // Skip over it
			    }
			}

			if (!LATServer::Instance()->is_local_service((char *)name))
			{
			    reply[num_replies]->remote_session = slotcmd->local_session;
			    reply[num_replies]->local_session = slotcmd->remote_session;
			    reply[num_replies]->length = 0;
			    reply[num_replies]->cmd = 0xD7; // No such service
			    num_replies++;
			}
			else
			{
			    std::string cmd;
			    int maxcon;
			    uid_t uid;
			    gid_t gid;
			    int curcon;
			    LATServer::Instance()->get_service_info((char *)name, cmd, maxcon, curcon, uid, gid);
			    strcpy((char *)servicename, (char *)name);

			    newsessionnum = next_session_number();
			    newsession = new ServerSession(*this,
							   (LAT_SessionStartCmd *)buf,
							   cmd, uid, gid,
							   slotcmd->remote_session,
							   newsessionnum, false);
			    if (newsession->new_session(remnode, "", (char *)portname,
							credits) == -1)
			    {
				newsession->send_disabled_message();
				delete newsession;
			    }
			    else
			    {
				sessions[newsessionnum] = newsession;
				// If we were pending a delete, we aren't now
				delete_pending = false;
			    }
			}
		    }
		}
		else // CLIENT
		{
		    if (session)
			((LATSession *)session)->got_connection(slotcmd->remote_session);
		}
		break;

	    case 0xa0:
		// Data_b message - port information
		if (session)
		    session->set_port((unsigned char *)slotcmd);
		break;

	    case 0xb0:
                // Attention. Force XON, Abort etc.
	        break;


	    case 0xc0:  // Reject - we will get disconnected
		debuglog(("Reject code %d: %s\n", credits,
			  lat_messages::session_disconnect_msg(credits)));
		// Deliberate fall-through.

	    case 0xd0:  // Disconnect
		if (session) session->disconnect_session(credits);

		// If we have no sessions left then disconnect
		if (num_clients() == 0)
		{
		    reply[num_replies]->remote_session = slotcmd->local_session;
		    reply[num_replies]->local_session = slotcmd->remote_session;
		    reply[num_replies]->length = 0;
		    reply[num_replies]->cmd = 0xD1;/* No more slots on circuit */
		    num_replies++;
		}
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

    // If "Response Requested" set, then make sure we send one.
    if (msg->header.cmd & 1 && !replyhere)
    {
	debuglog(("Sending response because we were told to (%x)\n", msg->header.cmd));
	replyhere = true;
    }

    // If the reply is just an ack then just set a flag
    // Then in circuit_timer, we send an ACK only if there is no DATA to send.
    if (replyhere && num_replies == 0)
	send_ack = true;

    // Send any replies
    if (replyhere && num_replies)
    {
	debuglog(("Sending %d slots in reply\n", num_replies));
	unsigned char replybuf[1600];

	memset(replybuf, 0, sizeof(replybuf));
	LAT_Header *header  = (LAT_Header *)replybuf;
	ptr = sizeof(LAT_Header);

	header->cmd       = LAT_CCMD_SREPLY;
	header->num_slots = num_replies;
	if (role == CLIENT) header->cmd |= 2; // To Host

	for (int i=0; i < num_replies; i++)
	{
	    memcpy(replybuf + ptr, reply[i], sizeof(LAT_SlotCmd));
	    ptr += sizeof(LAT_SlotCmd); // Already word-aligned
	}

	header->local_connid    = num;
	header->remote_connid   = remote_connid;

	pending_data.push(pending_msg(replybuf, ptr, false));

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
    response->latver           = LAT_VERSION;
    response->latver_eco       = LAT_VERSION_ECO;
    response->maxsessions      = 254;
    response->exqueued         = 0;
    response->circtimer        = LATServer::Instance()->get_circuit_timer();
    response->keepalive        = LATServer::Instance()->get_keepalive_timer();
    response->facility         = dn_htons(0);
    response->prodtype         = 3;   // Wot do we use here???
    response->prodver          = 3;   // and here ???

    ptr = sizeof(LAT_StartResponse);
    add_string(reply, &ptr, server->get_local_node());
    add_string(reply, &ptr, remnode);
    add_string(reply, &ptr, (unsigned char*)LATServer::greeting);
    reply[ptr++] = '\0';

    queue_message(reply, ptr);
}

// Send a message on this connection NOW
int LATConnection::send_message(unsigned char *buf, int len, send_type type)
{
    LAT_Header *response = (LAT_Header *)buf;

    response->local_connid    = num;
    response->remote_connid   = remote_connid;
    response->sequence_number = ++last_sent_seq;
    response->ack_number      = last_recv_seq;

    retransmit_count = 0;
    last_sent_ack = last_recv_seq;

    debuglog(("Sending message for connid %d (seq: %d, ack: %d) window=%d\n",
	      num, last_sent_seq, last_sent_ack, window_size ));

    keepalive_timer = 0;

    if (type == DATA)
    {
	need_ack = true;
	last_message = pending_msg(buf, len, true);
	window_size++;
    }
    else
    {
	need_ack = false;
	last_ack_message = pending_msg(buf, len, false);
    }

    return LATServer::Instance()->send_message(buf, len, interface, macaddr);
}

// Queue up a reply message
int LATConnection::queue_message(unsigned char *buf, int len)
{
    LAT_Header *response = (LAT_Header *)buf;

    response->local_connid    = num;
    response->remote_connid   = remote_connid;
    // SEQ and ACK filled in when we send it

    debuglog(("Queued data messsge for connid %d\n", num));

    pending_data.push(pending_msg(buf, len, true));
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

    // Delete server sessions
    for (unsigned int i=1; i<=highest_session; i++)
    {
	if (sessions[i])
	{
	    LATConnection *master = sessions[i]->get_master_conn();
	    if (master)
	    {
		ClientSession *cs = (ClientSession *)master->sessions[1];
		if (cs) cs->restart_pty();
	    }
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
	    highest_session = i;
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
    if (need_ack && last_sent_seq != last_recv_ack)
    {
	if (++retransmit_count > LATServer::Instance()->get_retransmit_limit())
	{
	    debuglog(("hit retransmit limit on connection %d\n", num));

	    unsigned char buf[1600];
	    LAT_Header *header = (LAT_Header *)buf;
	    int ptr = sizeof(LAT_Header);

	    header->cmd             = LAT_CCMD_CONREF;
	    if (role == CLIENT)
		header->cmd |= 2;
	    header->num_slots       = 0;
	    header->local_connid    = num;
	    header->remote_connid   = remote_connid;
	    header->sequence_number = ++last_sent_seq;
	    header->ack_number      = last_recv_seq;
	    buf[ptr++] = 0x06; // Retransmission limit reached.
	    LATServer::Instance()->send_message(buf, ptr, interface, macaddr);

	    // Set this connection pending deletion
	    LATServer::Instance()->delete_connection(num);

	    // Mark this node as unavailable in the service list
	    LATServices::Instance()->remove_node(std::string((char *)remnode));
	    return;
	}
	debuglog(("Last message not ACKed: RESEND\n"));
	last_message.send(interface, last_recv_seq, macaddr);
	return;
    }

    // If we're waiting for a non-flow-control message then
    // check for timeout.
    if (last_msg_type)
    {
	time_t tt;
	tt = time(NULL);

	if (tt - last_msg_time > 5)
	{
	    last_msg_time = tt;

	    // Too many retries ??
	    if (++last_msg_retries >= 3)
	    {
		LATServer::Instance()->delete_connection(num);
		last_msg_type = 0;
		return;
	    }
	    switch (last_msg_type)
	    {
		// Connect
	    case LAT_CCMD_CONNECT:
	        {
		    // Send another connect command
		    int ptr;
		    unsigned char buf[1600];
		    LAT_Start *msg = (LAT_Start *)buf;
		    ptr = sizeof(LAT_Start);

		    debuglog(("Resending connect to service on interface %d\n", interface));

		    msg->header.cmd          = LAT_CCMD_CONNECT;
		    msg->header.num_slots    = 0;
		    msg->header.local_connid = num;

		    msg->maxsize     = dn_htons(1500);
		    msg->latver      = LAT_VERSION;
		    msg->latver_eco  = LAT_VERSION_ECO;
		    msg->maxsessions = 254;
		    msg->exqueued    = 0;
		    msg->circtimer   = LATServer::Instance()->get_circuit_timer();
		    msg->keepalive   = LATServer::Instance()->get_keepalive_timer();
		    msg->facility    = dn_htons(0); // Eh?
		    msg->prodtype    = 3;   // Wot do we use here???
		    msg->prodver     = 3;   // and here ???

		    add_string(buf, &ptr, remnode);
		    add_string(buf, &ptr, LATServer::Instance()->get_local_node());
		    add_string(buf, &ptr, (unsigned char *)LATServer::greeting);
		    send_message(buf, ptr, DATA);
		    return;
		}
		break;

		// Request for queued connect
	    case LAT_CCMD_COMMAND:
                {
		    int ptr;
		    unsigned char buf[1600];
		    LAT_Command *msg = (LAT_Command *)buf;
		    ptr = sizeof(LAT_Command);

		    debuglog(("Resending queued connect to service on interface %d\n", interface));

		    msg->cmd         = LAT_CCMD_COMMAND;
		    msg->format      = 0;
		    msg->hiver       = LAT_VERSION;
		    msg->lover       = LAT_VERSION;
		    msg->latver      = LAT_VERSION;
		    msg->latver_eco  = LAT_VERSION_ECO;
		    msg->maxsize     = dn_htons(1500);
		    msg->request_id  = num;
		    msg->entry_id    = 0;
		    msg->opcode      = 2; // Request Queued connection
		    msg->modifier    = 1; // Send status periodically

		    add_string(buf, &ptr, remnode);

		    buf[ptr++] = 32; // Groups length
		    memcpy(buf + ptr, LATServer::Instance()->get_user_groups(), 32);
		    ptr += 32;

		    add_string(buf, &ptr, LATServer::Instance()->get_local_node());
		    buf[ptr++] = 0; // ASCIC source port
		    buf[ptr++] = 0; // add_string(buf, &ptr, (unsigned char *)LATServer::greeting);
		    add_string(buf, &ptr, servicename);
		    add_string(buf, &ptr, portname);

		    // Send it raw.
		    LATServer::Instance()->send_message(buf, ptr, interface, macaddr);
		    return;
		}
		break;

	    default:
		break;
	    }
	}
	return;
    }

    retransmit_count = 0;


    // Poll our sessions
    for (unsigned int i=0; i<=highest_session; i++)
    {
	if (sessions[i] && sessions[i]->isConnected())
	    sessions[i]->read_pty();
    }

    // Coalesce pending messages and queue them
    while (!slots_pending.empty())
    {
	debuglog(("circuit Timer:: slots pending = %d\n", slots_pending.size()));
        unsigned char buf[1600];
        LAT_Header *header = (LAT_Header *)buf;
        int len = sizeof(LAT_Header);

	if (role == SERVER)
	    header->cmd         = LAT_CCMD_SDATA;
	else
	    header->cmd         = LAT_CCMD_SESSION;
        header->num_slots       = 0;

	// Send as many slot data messages as we can
	while ( (header->num_slots < max_slots_per_packet && !slots_pending.empty()))
        {
            slot_cmd &cmd(slots_pending.front());

	    // make sure it fits
	    // do we really need max_slots_per_packet ?
	    if ((len + cmd.get_len()) > 1500)
		break;
	    header->num_slots++;

            memcpy(buf+len, cmd.get_buf(), cmd.get_len());
            len += cmd.get_len();
            if (len%2) len++;    // Keep it on even boundary

            slots_pending.pop();
        }

	if (header->num_slots)
	{
	    debuglog(("Collected %d slots on circuit timer\n", header->num_slots));
	    header->local_connid    = num;
	    header->remote_connid   = remote_connid;
	    pending_data.push(pending_msg(buf, len, true));
	}
    }

    // Send a reply if there are no data messages and the remote
    // end needs an ACK.
    if (send_ack &&
	(pending_data.empty() || window_size >= max_window_size))
    {
	debuglog(("Sending ACK reply\n"));
	unsigned char replybuf[1600];

	memset(replybuf, 0, sizeof(replybuf));
	LAT_Header *header  = (LAT_Header *)replybuf;
	int len = sizeof(LAT_Header);

	header->cmd       = LAT_CCMD_SREPLY;
	header->num_slots = 0;
	if (role == CLIENT) header->cmd |= 2; // To Host

	header->local_connid    = num;
	header->remote_connid   = remote_connid;

        header->sequence_number = last_sent_seq;
        header->ack_number      = last_recv_seq;

	send_message(replybuf, len, REPLY);
    }
    send_ack = false;

    //  Send a pending data message (if we can)
    if (!pending_data.empty() && window_size < max_window_size)
    {
        // Send the top message
        pending_msg &msg(pending_data.front());

	retransmit_count = 0;
	need_ack = msg.needs_ack();
	window_size++;

        LAT_Header *header      = msg.get_header();

	header->local_connid    = num;
	header->remote_connid   = remote_connid;
        header->sequence_number = ++last_sent_seq;
        header->ack_number      = last_recv_seq;

        debuglog(("Sending data message on circuit timer: seq: %d, ack: %d\n",
		  last_sent_seq, last_recv_seq));

        msg.send(interface, macaddr);
	last_message = msg; // Save it in case it gets lost on the wire;
        pending_data.pop();
	keepalive_timer = 0;
    }

    // Increment keepalive timer and trigger it if we are getting too close.
    // Keepalive timer is held in milli-seconds.

    // Of course, we needn't send keepalive messages when we are a
    // disconnected client.
    if (role == SERVER ||
	(role == CLIENT && connected))
    {
	struct timeval tv;
	long time_in_msec;

	// Initialise last_time for the first time
	if (last_time == 0)
	{
	    gettimeofday(&tv, NULL);
	    last_time = tv.tv_sec*1000 + tv.tv_usec/1000;
	}

	gettimeofday(&tv, NULL);
	time_in_msec = tv.tv_sec*1000 + tv.tv_usec/1000;

	keepalive_timer += time_in_msec - last_time;
	last_time = time_in_msec;

	if (keepalive_timer > (LATServer::Instance()->get_keepalive_timer()-3)*1000 )
	{
	    // Send an empty message that needs an ACK.
	    // If we don't get a response to this then we abort the circuit.
	    debuglog(("keepalive timer expired: %d: limit: %d\n", keepalive_timer,
		      LATServer::Instance()->get_keepalive_timer()*1000));

	    // If we get into this block then there is no chance that there is
	    // an outstanding ack (or if there is then it's all gone horribly wrong anyway)
	    // so it's safe to just send a NULL message out.
	    // If we do exqueued properly this may need revisiting.
	    send_ack = true;
	    return;
	}
    }
    // Delete us if delete_pending is set and no more data to send
    if (delete_pending && pending_data.empty() &&
	slots_pending.empty())
    {
	LAT_Header msg;

	debuglog(("Deleting pending connection\n"));
	msg.local_connid = remote_connid;
	msg.remote_connid = num;
	msg.sequence_number = ++last_sent_seq;
	msg.ack_number = last_recv_ack;
	LATServer::Instance()->send_connect_error(1, &msg, interface, macaddr);
	LATServer::Instance()->delete_connection(num);
    }

}

// Add as many data slots as we can to a reply.
int LATConnection::add_data_slots(int start_slot, LAT_SlotCmd *slots[4])
{
    int slot_num = start_slot;

    // Poll our sessions
    for (unsigned int i=0; i<=highest_session; i++)
    {
	if (sessions[i] && sessions[i]->isConnected())
	    sessions[i]->read_pty();
    }

// Overwrite any empty slot at the start
    LAT_SlotCmd *slot_header = (LAT_SlotCmd *)slots[0];
    if (start_slot == 1 &&
	slot_header->length == 0 &&
	!slots_pending.empty())
    {
	slot_num--;
    }

    while (slot_num < max_slots_per_packet && !slots_pending.empty())
    {
	slot_cmd &cmd(slots_pending.front());

	memcpy((char*)slots[slot_num], cmd.get_buf(), cmd.get_len());

	slots_pending.pop();
	slot_num++;
    }
    return slot_num;
}

void LATConnection::remove_session(unsigned char id)
{
    debuglog(("Deleting session %d\n", id));
    if (sessions[id])
    {
        delete sessions[id];
	sessions[id] = NULL;
    }

// Disconnect & remove connection if no sessions active...
// and no messages pending on circuit timer....
    if (num_clients() == 0)
    {

	if (pending_data.empty() && slots_pending.empty())
	{
	    LAT_Header msg;

	    msg.local_connid = remote_connid;
	    msg.remote_connid = num;
	    msg.sequence_number = ++last_sent_seq;
	    msg.ack_number = last_recv_ack;
	    LATServer::Instance()->send_connect_error(1, &msg, interface, macaddr);
	    LATServer::Instance()->delete_connection(num);
	}
	else
	{
	    // Otherwise just delete us when it's all calmed down.
	    delete_pending = true;
	}
    }
}

// Initiate a client connection
int LATConnection::connect(LATSession *session)
{
   // Look up the service name.
    std::string node;
    int  this_int=0;

    // Are we in the middle of actually connecting ?
    if (connecting) return 0;

    // Only connect if we are the first session,
    // else just initiate slot connect.
    if (!connected)
    {
	// If no node was specified then just use the highest rated one
	if (remnode[0] == '\0')
	{
	    if (!LATServices::Instance()->get_highest(std::string((char*)servicename),
						      node, macaddr, &this_int))
	    {
		debuglog(("Can't find service %s\n", servicename));
		// Tell the user
		session->disconnect_session(7);
		return -2; // Never eard of it!
	    }
	    strcpy((char *)remnode, node.c_str());
	}
	else
	{
	    // Try to find the node
	    if (!LATServices::Instance()->get_node(std::string((char*)servicename),
						   std::string((char*)remnode), macaddr, &this_int))
	    {
		debuglog(("Can't find node %s in service\n", remnode, servicename));

		// Tell the user
		session->disconnect_session(7);
		return -2; // Never eard of it!
	    }
	}

	// Reset the sequence & ack numbers
	last_recv_seq = 0xff;
	last_recv_ack = 0;
	last_sent_seq = 0xff;
	last_sent_ack = 0;
	remote_connid = 0;
	interface = this_int;
	connecting = true;

	// Queued connection or normal?
	if (queued)
	{
	    debuglog(("Requesting connect to queued service\n"));

	    int ptr;
	    unsigned char buf[1600];
	    LAT_Command *msg = (LAT_Command *)buf;
	    ptr = sizeof(LAT_Command);

	    msg->cmd         = LAT_CCMD_COMMAND;
	    msg->format      = 0;
	    msg->hiver       = LAT_VERSION;
	    msg->lover       = LAT_VERSION;
	    msg->latver      = LAT_VERSION;
	    msg->latver_eco  = LAT_VERSION_ECO;
	    msg->maxsize     = dn_htons(1500);
	    msg->request_id  = num;
	    msg->entry_id    = 0;
	    msg->opcode      = 2; // Request Queued connection
	    msg->modifier    = 1; // Send status periodically

	    add_string(buf, &ptr, remnode);

	    buf[ptr++] = 32; // Groups length
	    memcpy(buf + ptr, LATServer::Instance()->get_user_groups(), 32);
	    ptr += 32;

	    add_string(buf, &ptr, LATServer::Instance()->get_local_node());
	    buf[ptr++] = 0; // ASCIC source port
	    buf[ptr++] = 0; // add_string(buf, &ptr, (unsigned char *)LATServer::greeting);
	    add_string(buf, &ptr, servicename);
	    add_string(buf, &ptr, portname);

	    // Save the time we sent the connect so we
            // know if we got a response.
	    last_msg_time = time(NULL);
	    last_msg_type = msg->cmd;
	    last_msg_retries = 0;

	    // Send it raw.
	    return LATServer::Instance()->send_message(buf, ptr, interface, macaddr);
	}
	else
	{
	    int ptr;
	    unsigned char buf[1600];
	    LAT_Start *msg = (LAT_Start *)buf;
	    ptr = sizeof(LAT_Start);

	    debuglog(("Requesting connect to service on interface %d\n", interface));

	    msg->header.cmd          = LAT_CCMD_CONNECT;
	    msg->header.num_slots    = 0;
	    msg->header.local_connid = num;

	    msg->maxsize     = dn_htons(1500);
	    msg->latver      = LAT_VERSION;
	    msg->latver_eco  = LAT_VERSION_ECO;
	    msg->maxsessions = 254;
	    msg->exqueued    = 0;
	    msg->circtimer   = LATServer::Instance()->get_circuit_timer();
	    msg->keepalive   = LATServer::Instance()->get_keepalive_timer();
	    msg->facility    = dn_htons(0); // Eh?
	    msg->prodtype    = 3;   // Wot do we use here???
	    msg->prodver     = 3;   // and here ???

	    add_string(buf, &ptr, remnode);
	    add_string(buf, &ptr, LATServer::Instance()->get_local_node());
	    add_string(buf, &ptr, (unsigned char *)LATServer::greeting);

	    // Save the time we sent the connect so we
            // know if we got a response.
	    last_msg_time = time(NULL);
	    last_msg_type = msg->header.cmd;
	    last_msg_retries = 0;

	    return send_message(buf, ptr, DATA);
	}
    }
    else
    {
	// Just do a session connect.
	session->connect();
    }
    return 0;
}

// Initiate a reverse-LAT connection
int LATConnection::rev_connect()
{
    // Reset the sequence & ack numbers
    last_recv_seq = 0xff;
    last_recv_ack = 0;
    last_sent_seq = 0xff;
    last_sent_ack = 0;
    remote_connid = 0;
    connecting = true;

    int ptr;
    unsigned char buf[1600];
    LAT_Start *msg = (LAT_Start *)buf;
    ptr = sizeof(LAT_Start);

    debuglog(("REVLAT: Requesting connect to service on interface %d, request id = %d\n", interface, request_id));

    msg->header.cmd          = LAT_CCMD_CONNECT;
    msg->header.num_slots    = 0;
    msg->header.local_connid = num;

    msg->maxsize     = dn_htons(1500);
    msg->latver      = LAT_VERSION;
    msg->latver_eco  = LAT_VERSION_ECO;
    msg->maxsessions = 254;
    msg->exqueued    = 0;
    msg->circtimer   = LATServer::Instance()->get_circuit_timer();
    msg->keepalive   = LATServer::Instance()->get_keepalive_timer();
    msg->facility    = dn_htons(0); // Eh?
    msg->prodtype    = 3;   // Wot do we use here???
    msg->prodver     = 3;   // and here ???

    add_string(buf, &ptr, remnode);
    add_string(buf, &ptr, LATServer::Instance()->get_local_node());

    // Save the time we sent the connect so we
    // know if we got a response.
    last_msg_time = time(NULL);
    last_msg_type = msg->header.cmd;
    last_msg_retries = 0;

    return send_message(buf, ptr, DATA);
}

void LATConnection::got_status(unsigned char *node, LAT_StatusEntry *entry)
{
    debuglog(("Got status %d from node %s, queue pos = %d,%d. session: %d\n",
	      entry->status, node, entry->max_que_pos, entry->min_que_pos,
	      entry->session_id));

// Check this is OK - status session ID seems to be rubbish
    if (role == CLIENT && sessions[1])
    {
	last_msg_type = 0;
	last_msg_retries = 0;
	ClientSession *s = (ClientSession *)sessions[1];
	s->show_status(node, entry);
    }

// TODO: mark this session (connection?) as waiting for connect so 
// that, if the user quits, we can tell the server. (TODODO: what message ??)
}

int LATConnection::create_llogin_session(int fd, const char *service, const char *port, const char *localport,
					 const char *password)
{
// Create an lloginSession
    int newsessionnum = next_session_number();
    if (newsessionnum == -1)
	return -1;

    lloginSession *newsession = new lloginSession(*this, 0, newsessionnum,
						  (char *)localport, fd);
    if (newsession->new_session((unsigned char *)remnode, (char *)service, (char *)port, (char *)password, 0) == -1)
    {
	delete newsession;
	return -1;
    }
    sessions[newsessionnum] = newsession;
    return 0;
}

int LATConnection::create_localport_session(int fd, LocalPort *lport,
					    const char *service, const char *port,
					    const char *localport, const char *password)
{
// Create a localportSession for a /dev/lat port
    int newsessionnum = next_session_number();

    localportSession *newsession = new localportSession(*this, lport, 0, newsessionnum,
						  (char *)localport, fd);
    if (newsession->new_session((unsigned char *)remnode, (char *)service, (char *)port, (char *)password, 0) == -1)
    {
	delete newsession;
	return -1;
    }
    sessions[newsessionnum] = newsession;
    return 0;
}

// Create a server session for an incoming reverse-LAT connection
int LATConnection::create_reverse_session(const char *service,
					  const char *cmdbuf,
					  int reqid,
					  int ifn,
					  unsigned char *mac)
{
    std::string cmd;
    int maxcon;
    uid_t uid;
    gid_t gid;
    int curcon;

    debuglog(("Create reverse session for %s\n", service));

    if (LATServer::Instance()->get_service_info((char *)service, cmd, maxcon, curcon, uid, gid) == -1)
    {
	debuglog(("Service not known\n"));
	return -1;
    }

    int newsessionnum = next_session_number();

    request_id = reqid;
    interface = ifn;
    memcpy(macaddr, mac, 6);

    debuglog(("service %s, cmd = %s\n", service, cmd.c_str()));

    ReverseSession *newsession = new ReverseSession(*this,
						    (LAT_SessionStartCmd *)cmdbuf,
						    cmd,
						    uid,gid,
						    0,newsessionnum,
						    true);

    if (newsession->new_session((unsigned char *)remnode, (char *)remnode, (char *)"", 0) == -1)
    {
	delete newsession;
	return -1;
    }
    sessions[newsessionnum] = newsession;
    newsession->set_request_id(reqid);

    // Start it connecting.
    if (!connected && !connecting)
	rev_connect();

    if (connected)
	newsession->connect();
    return 0;
}

int LATConnection::got_connect_ack(unsigned char *buf)
{
    LAT_StartResponse *reply = (LAT_StartResponse *)buf;
    remote_connid = reply->header.local_connid;

    last_recv_ack = reply->header.ack_number;
    last_recv_seq = reply->header.sequence_number;

    connected = true;
    connecting = false;

    max_window_size = reply->exqueued+1;
    max_window_size = 1; // All we can manage
    window_size = 0;
//    need_ack = false;

    last_msg_type = 0;  // Not waiting anymore
    last_msg_retries = 0;

    debuglog(("got connect ack. seq: %d, ack: %d\n",
	      last_recv_seq, last_recv_ack));

// Start clientsessions
    for (unsigned int i=1; i<=highest_session; i++)
    {
	ClientSession *cs = (ClientSession *)sessions[i];
	if (cs && cs->waiting_start())
	{
	    cs->connect();
	}
    }
    return 0;
}

// Called when the client needs disconnecting
int LATConnection::disconnect_client()
{
// Reset all clients - remote end has disconnected the connection
    for (unsigned int i=0; i<=highest_session; i++)
    {
	ClientSession *cs = (ClientSession *)sessions[i];
	if (cs)
	{
	    cs->restart_pty();
	}
    }
    return 0;
}


// Extract "parameter 2" from the packet. If there is one then
// it means this is a connection from the terminal server for
// a queued *client* port so we...
// do "something clever..."
bool LATConnection::is_queued_reconnect(unsigned char *buf, int len, int *conn)
{
    int ptr = sizeof(LAT_SessionCmd)+3;

    ptr += buf[ptr]+1; // Skip over destination service
    if (ptr >= len) return false;

    ptr += buf[ptr]+1; // Skip over source service
    if (ptr >= len) return false;

// Do parameters -- look for a 2
    while (ptr < len)
    {
	int param_type = buf[ptr++];
	if (param_type == 2)
	{
	    ptr++; //Skip over parameter length (it's 2)
	    unsigned short param = dn_ntohs(*(unsigned short *)(buf+ptr));

	    debuglog(("found Parameter 2: request ID is %d\n", param));
	    *conn = param;
	    ptr +=2;
	    return true;
	}
	else
	{
	    ptr += buf[ptr]+1; // Skip over it
	}
    }
    return false;
}

int LATConnection::pending_msg::send(int interface, unsigned char *macaddr)
{
    return LATServer::Instance()->send_message(buf, len, interface, macaddr);
}

int LATConnection::num_clients()
{
    unsigned int i;
    int num = 0;

    for (i=1; i<=highest_session; i++)
	if (sessions[i]) num++;

    return num;
}
