/******************************************************************************
    (c) 2000-2002 Patrick Caulfield                 patrick@debian.org

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
#define MAX_INTERFACES 255
#include "interfaces.h"
class LATServer
{
    typedef enum {INACTIVE=0, LAT_SOCKET, LATCP_RENDEZVOUS, LLOGIN_RENDEZVOUS,
		  LATCP_SOCKET, LLOGIN_SOCKET, LOCAL_PTY, DISABLED_PTY} fd_type;

 public:
    static LATServer *Instance()
	{
	    if (!instance)
		return (instance = new LATServer());
	    else
		return instance;
	}

    void init(bool _static_rating, int _rating,
	      char *_service, char *_greeting, char **_interfaces,
	      int _verbosity, int _timer);
    void run();
    void shutdown();
    void add_fd(int fd, fd_type type);
    void remove_fd(int fd);
    void add_pty(LocalPort *port, int fd);
    void set_fd_state(int fd, bool disabled);
    int  send_message(unsigned char *buf, int len, int interface, unsigned char *macaddr);
    void delete_session(int, unsigned char, int);
    void delete_connection(int);
    unsigned char *get_local_node();
    int   get_circuit_timer()         { return circuit_timer; }
    int   get_retransmit_limit()      { return retransmit_limit; }
    void  set_retransmit_limit(int r) { retransmit_limit=r; }
    int   get_keepalive_timer()       { return keepalive_timer; }
    void  set_keepalive_timer(int k)  { keepalive_timer=k; }
    void  send_connect_error(int reason, LAT_Header *msg, int interface, unsigned char *macaddr);
    bool  is_local_service(char *);
    int   get_service_info(char *name, std::string &cmd, int &maxcon, int &curcon, uid_t &uid, gid_t &gid);
    gid_t get_lat_group() { return lat_group; }
    LATConnection **get_connection(int id) { return &connections[id]; }
    const unsigned char *get_user_groups() { return user_groups; }
    int   find_connection_by_node(const char *node);
    void  send_enq(unsigned char *);

    static unsigned char greeting[255];

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

    unsigned char local_name[256]; //  Node name
    int  interface_num[MAX_INTERFACES];
    int  interface_errs[MAX_INTERFACES];
    int  num_interfaces;
    unsigned char multicast_incarnation;
    int  verbosity;
    int  lat_socket;
    int  latcp_socket;
    int  llogin_socket;
    bool do_shutdown;
    bool locked;
    int  next_connection;
    gid_t lat_group;

    void  read_lat(int sock);
    double get_loadavg();
    void  reply_to_enq(unsigned char *inbuf, int len, int interface,
		      unsigned char *remote_mac);
    void  process_command_msg(unsigned char *inbuf, int len, int interface,
			      unsigned char *remote_mac);
    void  forward_status_messages(unsigned char *inbuf, int len);
    void  send_service_announcement(int sig);
    int   make_new_connection(unsigned char *buf, int len, int interface,
			      LAT_Header *header, unsigned char *macaddr);
    int   get_next_connection_number();

    void  add_services(unsigned char *, int, int, unsigned char *);
    void  got_enqreply(unsigned char *, int, int, unsigned char *);
    void  accept_latcp(int);
    void  accept_llogin(int);
    void  read_latcp(int);
    void  read_llogin(int);
    void  print_bitmap(std::ostrstream &, bool, unsigned char *bitmap);
    void  tidy_dev_directory();
    int   make_connection(int fd, const char *, const char *, const char *, const char *, const char *, bool);

    static void alarm_signal(int sig);

    class fdinfo
    {
    public:
	fdinfo(int _fd, LocalPort *_port, fd_type _type):
	    fd(_fd),
	    localport(_port),
	    type(_type)
	    {}

	int get_fd(){return fd;}
	LocalPort *get_localport(){return localport;}
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
	LocalPort *localport;
	fd_type type;
    };

    class deleted_session
    {
      private:
	fd_type type;
        int connection;
	unsigned char local_id;
	int fd;

      public:
	deleted_session(fd_type t, int c, unsigned char i, int f):
          type(t),
	  connection(c),
	  local_id(i),
	  fd(f)
	  {}
	int            get_conn(){return connection;}
	unsigned char  get_id()  {return local_id;}
	int            get_fd()  {return fd;}
	fd_type        get_type(){return type;}
    };

    // Class that defines a local (server) service
    class serviceinfo
    {
    public:
	serviceinfo(std::string n, int r, bool s, std::string i = std::string(""), int mc=0, char* comm="",
		    uid_t uid=0, gid_t gid=0):
	    name(n),
	    id(i),
	    command(std::string(comm)),
	    rating(r),
	    max_connections(mc),
	    cur_connections(0),
	    static_rating(s),
	    cmd_uid(uid),
	    cmd_gid(gid)
	    {
		if (command == std::string(""))
		    command = std::string(LOGIN_BIN);
	    }
	const std::string &get_name() {return name;}
	const std::string &get_id() {return id;}
	const std::string &get_command() {return command;}
	int           get_max_connections() {return max_connections;}
	int           get_cur_connections() {return cur_connections;}
	uid_t         get_uid() {return cmd_uid;}
	gid_t         get_gid() {return cmd_gid;}
	int           get_rating() {return rating;}
	bool          get_static() {return static_rating;}
	void          set_rating(int _new_rating, bool _static)
	    { rating = _new_rating; static_rating = _static; }
	void          set_ident(char *_ident)
	    { id = std::string(_ident);}
	void inc_connections() {cur_connections++;}
	void dec_connections() {cur_connections--;}

	const bool operator==(serviceinfo &si)  { return (si == name);}
	const bool operator==(const std::string &nm) { return (nm == name);}
	const bool operator!=(serviceinfo &si)  { return (si != name);}
	const bool operator!=(const std::string &nm) { return (nm != name);}

    private:
	std::string name;
	std::string id;
	std::string command;
	int rating;
	int max_connections;
	int cur_connections;
	bool static_rating;
	uid_t cmd_uid;
	gid_t cmd_gid;
    };

    void process_data(fdinfo &);
    void delete_entry(deleted_session &);
    void interface_error(int, int);

    // Constants
    static const int MAX_CONNECTIONS = 255;

    // Collections
    std::list<fdinfo>          fdlist;
    std::list<deleted_session> dead_session_list;
    std::list<int>             dead_connection_list;
    std::list<serviceinfo>     servicelist;
    std::list<LocalPort>       portlist;

    // Connections indexed by ID
    LATConnection *connections[MAX_CONNECTIONS];

    // LATCP connections
    std::map<int, Circuit*> latcp_circuits;

    // LATCP configurable parameters
    int           circuit_timer;   // Default 8 (=80 ms)
    int           multicast_timer; // Default 60 (seconds)
    int           retransmit_limit;// Default 20
    int           keepalive_timer; // Default 20 (seconds)
    bool          responder;       // Be a service responder (false);
    unsigned char groups[32];      // Bitmap of groups
    bool          groups_set;      // Have the server groups been set ?
    unsigned char user_groups[32]; // Bitmap of user groups..always in use
    LATinterfaces *iface;

    // LATCP Circuit callins
 public:
    void SetResponder(bool onoff) { responder = onoff;}
    void Shutdown();
    bool add_service(char *name, char *ident, char *command,
		     int maxcon, uid_t uid, gid_t gid, int _rating, bool _static_rating);
    bool set_rating(char *name, int _rating, bool _static_rating);
    bool set_ident(char *name, char *ident);
    bool remove_service(char *name);
    bool remove_port(char *name);
    void set_multicast(int newtime);
    void set_nodename(unsigned char *);
    void unlock();
    bool show_characteristics(bool verbose, std::ostrstream &output);
    int  create_local_port(unsigned char *, unsigned char *,
			   unsigned char *, unsigned char *, bool, bool,
			   unsigned char *);
    int  make_llogin_connection(int fd, const char *, const char *, const char *, const char *, const char *, bool);
    int  make_port_connection(int fd, LocalPort *, const char *, const char *, const char *,
			      const char *, const char *, bool);
    int  set_servergroups(unsigned char *bitmap);
    int  unset_servergroups(unsigned char *bitmap);
    int  set_usergroups(unsigned char *bitmap);
    int  unset_usergroups(unsigned char *bitmap);
};
