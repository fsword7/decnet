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

class LATCPCircuit
{
    public:
    LATCPCircuit(): fd(-1) {}
    LATCPCircuit(int _fd);

    ~LATCPCircuit();
    
    bool do_command();

 private:
    int fd;
    enum {STARTING, RUNNING} state;

    bool send_reply(int, char *, int);
};
