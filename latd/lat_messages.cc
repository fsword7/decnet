/******************************************************************************
    (c) 2000 Patrick Caulfield                 patrick@debian.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
******************************************************************************/

#include "lat_messages.h"

char *lat_messages::session_disconnect_msg(int code)
{
    switch (code)
    {
    default:
    case 0x0: return "Unknown";
	break;
    case 0x1: return "User requested disconnect";
	break;
    case 0x2: return "System shutdown in progress";	
	break;
    case 0x3: return "Invalid slot received";
	break;
    case 0x4: return "Invalid service class";
	break;
    case 0x5: return "Insufficient resources";
	break;
    case 0x6: return "Service in use";
	break;
    case 0x7: return "No such service";
	break;
    case 0x8: return "Service is disabled";
	break;
    case 0x9: return "Service is not offered by requested port";
	break;
    case 0xa: return "Port name is unknown";
	break;
    case 0xb: return "Invalid password";
	break;
    case 0xc: return "Entry is not in the queue";
	break;
    case 0xd: return "Immediate access rejected";
	break;
    case 0xe: return "Access denied";
	break;
    case 0xf: return "Corrupted solicit request";
	break;
    }
}

char *lat_messages::connection_disconnect_msg(int code)
{
    switch(code)
    {
    default:
    case 0x00: return "Unknown";
	break;
    case 0x01: return "No more slots on circuit";
	break;
    case 0x02: return "Illegal message or slot format received";
	break;
    case 0x03: return "VC_Halt from user";
	break;
    case 0x04: return "No progress being made";
	break;
    case 0x05: return "Time limit expired";
	break;
    case 0x06: return "Retransmission limit reached";
	break;
    case 0x07: return "Insufficient resources to satisfy request";
	break;
    case 0x08: return "Server circuit timer out of range";
	break;
    case 0x09: return "Number of virtual circuits exceeded";
	break;
    }
}
