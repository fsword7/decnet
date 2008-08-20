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


class QueuedSession: public ServerSession
{
 public:
  QueuedSession(class LATConnection &p,
	      LAT_SessionStartCmd *cmd,
	      ClientSession *_client,
	      unsigned char remid, unsigned char localid, bool clean);
  virtual ~QueuedSession();

  virtual int new_session(unsigned char *remote_node,
			  char *service, char *port, unsigned char c);

 private:
  ClientSession *client_session;
};
