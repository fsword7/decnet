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

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#include <sstream>
#include <list>
#include <string>
#include <map>
#include <queue>
#include <iterator>

#include "lat.h"
#include "latcp.h"
#include "utils.h"
#include "services.h"
#include "session.h"
#include "localport.h"
#include "connection.h"
#include "circuit.h"
#include "latcpcircuit.h"
#include "server.h"


bool Circuit::send_reply(int cmd, const char *buf, int len)
{
    char outhead[3];

    if (len == -1) len=strlen(buf)+1;
    
    outhead[0] = cmd;
    outhead[1] = len/256;
    outhead[2] = len%256;
    if (write(fd, outhead, 3) != 3) return false;
    if (write(fd, buf, len) != len) return false;
    return true;
}

