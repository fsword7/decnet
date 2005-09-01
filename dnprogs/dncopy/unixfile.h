
class unixfile: public file
{
 public:
    unixfile(char *name);
    unixfile();
    ~unixfile();

    virtual int setup_link(unsigned int bufsize, int rfm, int rat, int xfer_mode, int flags);
    
    virtual int   open(char *mode);
    virtual int   open(char *basename, char *mode);
    virtual int   close();
    virtual int   read(char *buf,  int len);
    virtual int   write(char *buf, int len);
    virtual int   next();
    virtual void  perror(char *);
    virtual char *get_basename(int keep_version);
    virtual char *get_printname();
    virtual char *get_printname(char *filename);
    virtual char *get_format_name();
    virtual int   get_umask();
    virtual int   set_umask(int mask);
    virtual bool  eof();
    virtual bool  isdirectory();
    virtual bool  iswildcard();
    virtual int   max_buffersize(int biggest);    

 protected:
    char   filename[MAX_PATH+1];
    char   printname[MAX_PATH+1];
    FILE  *stream;
    int    user_rfm;
    int    user_rat;
    int    transfer_mode;
    char  *record_buffer;
    int    record_ptr;
    int    record_buflen;
    unsigned int block_size;

    static const int RECORD_BUFSIZE = 4096;
};

