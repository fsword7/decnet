
class LATSession
{
 public:
    LATSession(class LATConnection &p, unsigned char remid, unsigned char localid):
	pid(-1),
	parent(p),
	remote_session(remid),
	local_session(localid),
	max_read_size(255),
	credit(0),
	stopped(false),
	remote_credit(0)
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

    virtual void disconnect_session(int reason);
    virtual int new_session(unsigned char *_remote_node, unsigned char c)=0;
    virtual void do_read() {}
    
 protected:
    enum {STARTING, LOGIN, RUNNING, STOPPED} state;
    char           remote_node[32]; // Node name
    char           ptyname[256];
    int            master_fd;
    pid_t          pid;
    bool           echo_expected;
    bool           connected;
    class LATConnection &parent;
    unsigned char  remote_session;
    unsigned char  local_session;
    int            max_read_size;
    
    // Flow control
    int            credit;
    bool           stopped;
    int            remote_credit;

 protected:
    int  send_data(unsigned char *buf, int msglen, int );
    void send_issue();
    void add_slot(unsigned char *buf, int &ptr, int slotcmd, unsigned char *slotdata, int len);
};
