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


class LATConnection
{
 public:

    typedef enum {REPLY, DATA, CONTINUATION} send_type;
  
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
    void send_slot_message(unsigned char *, int);
    void circuit_timer();
    void remove_session(unsigned char);


    // Client session routines
    LATConnection(int _num,
		  const char *_service, const char *_portname,
		  const char *_lta, const char *remnode,
		  bool queued, bool clean);
    int connect(class ClientSession *);
    int create_client_session(char *, char *);
    int create_llogin_session(int, char *service, char *port);
    int disconnect_client();              // From LATServer
    int got_connect_ack(unsigned char *); // Callback from LATServer
    bool isClient() { return role==CLIENT;}
    const char *getLocalPortName() { return lta_name; }
    void show_client_info(bool verbose, std::ostrstream &);
    int get_connection_id() { return num;}
    void got_status(unsigned char *node, LAT_StatusEntry *entry);
    bool node_is(char *node) { return strcmp(node, (char *)remnode)==0;}
    int  num_clients();
    
 private:
    int            num;           // Local connection ID
    int            interface;     // Ethernet i/f we are using
    int            remote_connid; // Remote Connection ID
    int            keepalive_timer; // Counting up.
    unsigned char  last_sequence_number;
    unsigned char  last_ack_number;
    unsigned int   next_session;
    unsigned char  macaddr[6];
    unsigned char  servicename[255];
    unsigned char  portname[255];
    unsigned char  remnode[255];
    LATSession    *sessions[256];

    unsigned char  last_sent_sequence; // Last sequence number we sent
    unsigned char  last_message_acked;
    bool           need_ack;
    bool           queued;             // Client for queued connection.
    bool           queued_slave;       // We are a slave connection for a queued client
    bool           eightbitclean;
    bool           connected;
    LATConnection *master_conn;        // Client connection we are slave to
    bool           connecting;
    
    int next_session_number();
    void send_a_reply(unsigned char local_session, unsigned char remote_session);
    bool is_queued_reconnect(unsigned char *buf, int len, int *conn);
	
    static const unsigned int MAX_SESSIONS = 254;
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
      int send(int interface, unsigned char seq, unsigned char *macaddr)
      {
	  LAT_Header *header = (LAT_Header *)buf;
	  header->sequence_number = seq;
	  return send(interface, macaddr);
      }
      LAT_Header *get_header() { return (LAT_Header *)buf;}
      bool needs_ack() { return need_ack;}
      
    private:
      unsigned char buf[1600];
      int   len;
      bool  need_ack;
    };

    std::queue<pending_msg> pending;

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
    int retransmit_count;

    // name of client device to create
    char lta_name[255];
};
