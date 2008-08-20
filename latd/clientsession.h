/******************************************************************************
    (c) 2000 Christine Caulfield                 christine.caulfield@googlemail.com

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
		unsigned char remid, unsigned char localid, char *, bool clean);

  virtual ~ClientSession();
  virtual int new_session(unsigned char *remote_node,
			  char *service, char *port, unsigned char c);
  virtual void do_read();
  virtual void disconnect_session(int reason);

  virtual int  connect_parent();
  virtual void restart_pty();
  virtual void show_status(unsigned char *node, LAT_StatusEntry *entry);
  virtual void start_port();
  int get_port_fd();

 private:
  int slave_fd;
  bool slave_fd_open;
  char mastername[255];
};
