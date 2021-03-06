/******************************************************************************
    (c) 2001-2002 Christine Caulfield                 christine.caulfield@googlemail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
******************************************************************************/

class LLOGINCircuit: public Circuit
{
    public:
    LLOGINCircuit(int _fd);

    virtual ~LLOGINCircuit();

    virtual bool do_command();

 private:
    enum {STARTING, RUNNING, TERMINAL} state;
    char msg[1024];
    char node[256];
    char service[256];
    char port[256];
    char localport[256];
    char password[256];
};
