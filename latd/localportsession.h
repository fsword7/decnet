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

class localportSession: public lloginSession
{
 public:
    localportSession(class LATConnection &p, class LocalPort *port,
		     unsigned char remid, unsigned char localid, char *lta, int);

    virtual ~localportSession();
    virtual void do_read();
    
 private:
    LocalPort *localport;


    int minimum_read;
    bool ignored_read;
};
