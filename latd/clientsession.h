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

class ClientSession: public LATSession
{
 public:
  ClientSession(class LATConnection &p,
		unsigned char remid, unsigned char localid, char *);

  virtual ~ClientSession();
  virtual int new_session(unsigned char *remote_node, unsigned char c);
  virtual void do_read();
  virtual void disconnect_session(int reason);
    
  void connect(char *service, char *port);
  void restart_pty();
  int  connect_parent();
  void got_connection(unsigned char);
  
 private:
  int slave_fd;
  bool slave_fd_open;
  char mastername[255];
  char ltaname[255];
};
