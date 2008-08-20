/******************************************************************************
    (c) 2001 Christine Caulfield                 christine.caulfield@googlemail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
******************************************************************************/

class LocalPort
{
 public:
    LocalPort(unsigned char *service, unsigned char *portname, unsigned char *devname,
	      unsigned char *remnode, bool queued, bool clean, unsigned char *password);

    LocalPort(const LocalPort &p);
    ~LocalPort();

  void do_read();
  void disconnect_session(int reason);
  void restart_pty();
  int get_port_fd();
  void show_info(bool verbose, std::ostringstream &output);
  void close_and_delete();
  const std::string &get_devname() { return devname; };
  void init_port();

 private:

  bool connect_session();

  std::string service;
  std::string portname;
  std::string devname;
  std::string remnode;
  std::string password;
  bool queued;
  bool clean;
  bool slave_fd_open;
  bool connected;
  char ptyname[255];

  int master_fd;
  int slave_fd;
};
