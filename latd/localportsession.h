/******************************************************************************
    (c) 2001-2003 Christine Caulfield                 christine.caulfield@googlemail.com

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
    virtual void show_status(unsigned char *node, LAT_StatusEntry *entry);


 private:
    LocalPort *localport;


    int minimum_read;
    bool ignored_read;
};
