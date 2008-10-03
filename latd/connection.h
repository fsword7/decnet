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


class LATConnection
{
 public:

    typedef enum {REPLY, DATA, CONTINUATION} send_type;
    static const unsigned int MAX_SESSIONS = 4;

    LATConnection(int _num, unsigned char *buf, int len,
		  int _interface,
		  unsigned char _seq,
		  unsigned char _ack,
		  unsigned char *_macaddr);
    ~LATConnection();
    bool process_session_cmd(unsigned char *, int, unsigned char *);
    void send_connect_ack();
    int  send_message(unsigned char *, int, send_type);
    int  queue_message(unsigned char *, int);
    int  add_data_slots(int start_slot, LAT_SlotCmd *slots[4]);
    void send_slot_message(unsigned char *, int);
    void circuit_timer();
    void remove_session(unsigned char);


    // Client session routines
    LATConnection(int _num,
		  const char *_service, const char *_portname,
		  const char *_lta, const char *remnode,
		  bool queued, bool clean);
    int connect(class LATSession *);
    int rev_connect();
    int create_llogin_session(int, const char *service, const char *port, const char *localport, const char *password);
    int create_reverse_session(const char *, const char *, int, int,
			      unsigned char *);
    int create_localport_session(int, class LocalPort *, const char *service,
				 const char *port, const char *localport,
				 const char *password);
    int disconnect_client();              // From LATServer
    int got_connect_ack(unsigned char *); // Callback from LATServer
    bool isClient() { return role==CLIENT;}
    const char *getLocalPortName() { return lta_name; }
    int get_connection_id() { return num;}
    void got_status(unsigned char *node, LAT_StatusEntry *entry);
    bool node_is(const char *node) { return strcmp(node, (char *)remnode)==0;}
    unsigned int  num_clients();
    const char *get_servicename() { return (const char *)servicename; }

 private:
    int            num;           // Local connection ID
    int            interface;     // Ethernet i/f we are using
    int            remote_connid; // Remote Connection ID
    int            keepalive_timer; // Counting up.
    unsigned char  last_sent_seq;
    unsigned char  last_sent_ack;
    unsigned char  last_recv_seq;
    unsigned char  last_recv_ack;
    unsigned int   next_session;
    unsigned int   highest_session;
    unsigned char  macaddr[6];
    unsigned char  servicename[255];
    unsigned char  portname[255];
    unsigned char  remnode[255];
    unsigned long  last_time;
    LATSession    *sessions[256];

    bool           need_ack;           // The message we last sent requires an ACK
    bool           queued;             // Client for queued connection.
    bool           eightbitclean;
    bool           connected;
    bool           connecting;
    bool           delete_pending;
    unsigned short request_id;         // For incoming reverse-LATs

    // Keep track of non-flow-controlled messages
    time_t         last_msg_time;
    int            last_msg_type;
    int            last_msg_retries;

    int next_session_number();
    bool is_queued_reconnect(unsigned char *buf, int len, int *conn);

    enum {CLIENT, SERVER} role;

    // A class for keeping pending messages.
    class pending_msg
    {
    public:
      pending_msg() {}
      pending_msg(unsigned char *_buf, int _len, bool _need_ack):
	  len(_len),
	  need_ack(_need_ack)
	{
	    memcpy(buf, _buf, len);
	}
      pending_msg(const pending_msg &msg):
	  len(msg.len),
	  need_ack(msg.need_ack)
      {
	  memcpy(buf, msg.buf, len);
      }
      ~pending_msg()
       {
       }

      pending_msg &operator=(const pending_msg &a)
      {
	  if (&a != this)
	  {
	      len = a.len;
	      need_ack = a.need_ack;
	      memcpy(buf, a.buf, len);
	  }
	  return *this;
      }

      int send(int interface, unsigned char *macaddr);
      int send(int interface, unsigned char ack, unsigned char *macaddr)
      {
	  LAT_Header *header = (LAT_Header *)buf;
	  header->ack_number = ack;
	  return send(interface, macaddr);
      }
      LAT_Header *get_header() { return (LAT_Header *)buf;}
      bool needs_ack() { return need_ack;}
      unsigned char get_seq() { LAT_Header *h=(LAT_Header *)buf; return h->sequence_number;}

    private:
      unsigned char buf[1600];
      int   len;
      bool  need_ack;
    };

    // Queue of pending DATA messages
    std::queue<pending_msg> pending_data;

    // This class & queue is for the slot messages. we coalesce these
    // into a "real" message when the crcuit timer triggers.
    class slot_cmd
    {
    public:
      slot_cmd(unsigned char *_buf, int _len):
	len(_len)
	{
	    memcpy(buf, _buf, len);
	}
      slot_cmd(const slot_cmd &cmd)
	{
	    len = cmd.len;
	    memcpy(buf, cmd.buf, len);
	}

      ~slot_cmd()
	{
	}

      int   get_len(){return len;}
      unsigned char *get_buf(){return buf;}
      LAT_SlotCmd   *get_cmd(){return (LAT_SlotCmd *)buf;}

    private:
        unsigned char  buf[300];
        int            len;
    };

    std::queue<slot_cmd> slots_pending;

    int max_window_size;  // As set by the client.
    int window_size;      // Current window size
    int lat_eco;          // Remote end's LAT ECO version
    int max_slots_per_packet;
    pending_msg last_message; // In case we need to resend it.
    pending_msg last_ack_message; // In case we need to resend it.
    int retransmit_count;

    // name of client device to create
    char lta_name[255];

    // Whether we need to send an ACK if there are no data messages
    // available at the next circuit timer tick
    bool send_ack;
};
