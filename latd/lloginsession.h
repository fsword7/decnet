/******************************************************************************
    (c) 2001 Patrick Caulfield                 patrick@debian.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
******************************************************************************/

class lloginSession: public ClientSession
{
 public:
  lloginSession(class LATConnection &p,
		unsigned char remid, unsigned char localid, int);

  virtual ~lloginSession();
  virtual int  new_session(unsigned char *remote_node, unsigned char c);
  virtual void do_read();
  virtual void disconnect_session(int reason);
    
  virtual void connect(char *service, char *port);
  virtual int  connect_parent();
  virtual void got_connection(unsigned char);
  virtual int  get_port_fd();
  virtual void restart_pty() {disconnect_sock();};


 private:
  void disconnect_sock();

};
