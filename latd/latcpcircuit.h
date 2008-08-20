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

class LATCPCircuit: public Circuit
{
    public:
    LATCPCircuit(): Circuit(-1) {}
    LATCPCircuit(int _fd);

    virtual ~LATCPCircuit();
    
    virtual bool do_command();

 protected:
    enum {STARTING, RUNNING} state;
};
