
class LATSession
{
 public:
    LATSession(class LATConnection &p,
	       unsigned char remid, unsigned char localid, bool _clean):
	pid(-1),
	parent(p),
	remote_session(remid),
	local_session(localid),
	max_read_size(250),
	clean(_clean),
	credit(0),
	request_id(0),
	stopped(false),
	remote_credit(0),
	master_conn(NULL)
      {}
    virtual ~LATSession();

    int  send_data_to_process(unsigned char *buf, int len);
    int  read_pty();
    void remove_session();
    void send_disabled_message();
    void add_credit(signed short c);
    void set_port(unsigned char *inbuf);
    int  get_remote_credit() { return remote_credit; }
    void inc_remote_credit(int inc) { remote_credit+=inc; }
    void got_connection(unsigned char _remid);
    bool isConnected() { return connected; }
    bool waiting_start() { return state == STARTING; }

    virtual void disconnect_session(int reason);
    virtual int new_session(unsigned char *_remote_node,
			    char *service, char *port, unsigned char c)=0;
    virtual void do_read() {}
    virtual void connect();

    // These two are for queuedsession really, but we don't
    // know what type of session we have in the connection dtor
    void set_master_conn(LATConnection **master)
	{
	    master_conn = master;
	}
    LATConnection *get_master_conn()
	{
	    if (master_conn)
	        return *master_conn;
	    else
		return NULL;
	}

 protected:
    enum {NEW, STARTING, RUNNING, STOPPED} state;
    char           remote_node[32]; // Node name
    char           remote_service[32]; // Service name
    char           remote_port[32];
    char           ptyname[256];
    int            master_fd;
    pid_t          pid;
    bool           echo_expected;
    bool           connected;
    class LATConnection &parent;
    unsigned char  remote_session;
    unsigned char  local_session;
    int            max_read_size;
    bool           clean; // Connection should be 8bit clean
    char           ltaname[255];
    unsigned short request_id;

    // Flow control
    int            credit;
    bool           stopped;
    int            remote_credit;
    LATConnection  **master_conn;


 protected:
    int  send_data(unsigned char *buf, int msglen, int );
    void send_issue();
    int  send_break();
    void add_slot(unsigned char *buf, int &ptr, int slotcmd, unsigned char *slotdata, int len);
    void crlf_to_lf(unsigned char *buf, int len, unsigned char *newbuf, int *newlen);
    int  writeall(int fd, unsigned char *buf, int len);
};
