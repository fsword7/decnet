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
// Singleton server object

class LATServer
{
    typedef enum {INACTIVE=0, LAT_SOCKET, LATCP_RENDEZVOUS,
		  LATCP_SOCKET, LOCAL_PTY, DISABLED_PTY} fd_type;
    
 public:
    static LATServer *Instance()
	{
	    if (!instance)
		return (instance = new LATServer());
	    else
		return instance;
	}
    
    void init(bool _static_rating, int _rating,
	      char *_service, char *_greeting, int _interface_num,
	      int _verbosity, int _timer, char *_our_macaddr);
    void run();
    void shutdown();
    void add_fd(int fd, fd_type type);
    void remove_fd(int fd);
    void add_pty(LATSession *session, int fd);
    void set_fd_state(int fd, bool disabled);
    int  send_message(unsigned char *buf, int len, unsigned char *macaddr);
    void delete_session(LATConnection *, unsigned char, int);
    void delete_connection(int);
    unsigned char *get_local_node();
    int   get_circuit_timer()         { return circuit_timer; }
    int   get_retransmit_limit()      { return retransmit_limit; }
    void  set_retransmit_limit(int r) { retransmit_limit=r; }
    int   get_keepalive_timer()       { return keepalive_timer; }
    void  set_keepalive_timer(int k)  { keepalive_timer=k; }
    
 private:
    LATServer():
	circuit_timer(8),
	multicast_timer(60),
	retransmit_limit(20),
	keepalive_timer(20),
	responder(false)
      {};                       // Private constructor to force singleton
    static LATServer *instance; // Singleton instance

    // These two are defaults for new services added
    bool static_rating;
    int  rating;

    unsigned char greeting[255];
    unsigned char our_macaddr[6];
    unsigned char multicast_incarnation;
    int  interface_num;
    int  verbosity;
    int  lat_socket;
    int  latcp_socket;
    bool do_shutdown;
    int  next_connection;

    void  read_lat(int sock);
    float get_loadavg();
    void  reply_to_enq(unsigned char *inbuf, int len,
		      unsigned char *remote_mac);
    void  send_service_announcement(int sig);
    int   make_new_connection(unsigned char *buf, int len, LAT_Header *header, unsigned char *macaddr);
    int   get_next_connection_number();

    void  add_services(unsigned char *, int, unsigned char *);
    void  accept_latcp(int);
    void  read_latcp(int);
    void  send_connect_error(int reason, LAT_Header *msg, unsigned char *macaddr);
 
    static void alarm_signal(int sig);

    class fdinfo
    {
    public:
	fdinfo(int _fd, LATSession *_session, fd_type _type):
	    fd(_fd),
	    session(_session),
	    type(_type)
	    {}

	int get_fd(){return fd;}
	LATSession *get_session(){return session;}
	fd_type get_type(){return type;}
	void set_disabled(bool d)
	    {
		if (d)
		    type = DISABLED_PTY;
		else
		    type = LOCAL_PTY;
		
	    }

	bool active()
	{
	  return (!(type == INACTIVE || type == DISABLED_PTY));
	}
	
	bool operator==(int _fd)
	{
	    return (type != INACTIVE && fd == _fd);
	}

	bool operator==(const fdinfo &fdi)
	{
	    return (fd == fdi.fd);
	}

	bool operator!=(const fdinfo &fdi)
	{
	    return (fd != fdi.fd);
	}

	bool operator!=(int _fd)
	{
	    return (type == INACTIVE || fd != _fd);
	}
	
    private:
	int  fd;
	LATSession *session;
	fd_type type;
    };

    class deleted_session
    {
      private:
	fd_type type;
        LATConnection *connection;
	unsigned char local_id;
	int fd;

      public:
	deleted_session(fd_type t, LATConnection *c, unsigned char i, int f):
          type(t),
	  connection(c),
	  local_id(i),
	  fd(f)
	  {}
	LATConnection* get_conn(){return connection;}
	unsigned char  get_id()  {return local_id;}
	int            get_fd()  {return fd;}
	fd_type        get_type(){return type;}
    };

    class serviceinfo
    {
    public:
	serviceinfo(string n, int r, bool s, string i = string("") ):
	    name(n),
	    id(i),
	    rating(r),
	    static_rating(s)
	    {}
	const string &get_name() {return name;}
	const string &get_id() {return id;}
	int           get_rating() {return rating;}
	bool          get_static() {return static_rating;}
	
	const bool operator==(serviceinfo &si)  { return (si == name);}
	const bool operator==(const string &nm) { return (nm == name);}
	const bool operator!=(serviceinfo &si)  { return (si != name);}
	const bool operator!=(const string &nm) { return (nm != name);}

    private:
	string name;
	string id;
	int rating;
	bool static_rating;
    };
    
    void process_data(fdinfo &);
    void delete_entry(deleted_session &);
    
    // Constants
    static const int MAX_CONNECTIONS = 5;

    // Collections
    list<fdinfo>          fdlist;
    list<deleted_session> dead_session_list;
    list<int>             dead_connection_list;
    list<serviceinfo>     servicelist;
    
    // Connections indexed by ID
    LATConnection *connections[MAX_CONNECTIONS];

    // LATCP connections
    map<int, LATCPCircuit> latcp_circuits;

    // LATCP configurable parameters
    int  circuit_timer;   // Default 8 (=80 ms)
    int  multicast_timer; // Default 60 (seconds)
    int  retransmit_limit;// Default 20
    int  keepalive_timer; // Default ??
    bool responder;       // Be a service responder (false);

    // LATCP Circuit callins
 public:
    void SetResponder(bool onoff) { responder = onoff;}
    void Shutdown();
    void add_service(char *name, char *ident);
    bool remove_service(char *name);
    bool remove_port(char *name);
    void set_multicast(int newtime);
    bool show_characteristics(bool verbose, ostrstream &output);
    int  make_client_connection(unsigned char *, unsigned char *,
				unsigned char *, unsigned char *, bool);
};
