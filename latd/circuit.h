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
#include <stdio.h>
class Circuit
{
    public:
    Circuit(): fd(-1) {}
    Circuit(int _fd): fd(_fd) {};

    virtual ~Circuit() {};
    
    virtual bool do_command() { return false;};

 protected:
    int fd;

    bool send_reply(int, char *, int);
};
