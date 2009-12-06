/******************************************************************************
    (c) 2002-2003 Christine Caulfield                 christine.caulfield@googlemail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
******************************************************************************/

// interfaces.h

// Abstract out the linux-specific ioctls and interface stuff.
//
// To port latd, all you should need to do is extend this class with
// your platform-specific functions.

// See linux-interface.cc for example implementation.

#include <cstring>
#include <iostream>

class LATinterfaces
{
 protected:

    // Save a copy of the protocol
    int protocol;

 public:

    LATinterfaces();
    virtual ~LATinterfaces();

    // Initialise the LAT protocol
    virtual int Start(int proto)=0;

    // Return a list of valid interface numbers and the count
    virtual void get_all_interfaces(int ifs[], int &num)=0;

    // Print the name of an interface
    virtual std::string ifname(int ifn) = 0;

    // Find an interface number by name
    virtual int find_interface(char *name)=0;

    // true if this class defines one FD for each active
    // interface, false if one fd is used for all interfaces.
    virtual bool one_fd_per_interface()=0;

    // Return the FD for this interface (will only be called once for
    // select if above returns false
    virtual int get_fd(int ifn)=0;

    // Send a packet to a given macaddr
    virtual int send_packet(int ifn, unsigned char macaddr[], unsigned char *data, int len)=0;

    // Receive a packet from a given FD (note FD not iface)
    virtual int recv_packet(int fd, int &ifn, unsigned char macaddr[], unsigned char *data, int maxlen, bool &more)=0;

    // Enable reception of LAT multicast messages
    virtual int set_lat_multicast(int ifn)=0;

    // Finished listening for LAT multicasts
    virtual int remove_lat_multicast(int ifn)=0;

    // Bind a socket to an interface
    virtual int bind_socket(int interface)=0;

    // Creates a platform-specifc interfaces class
    static LATinterfaces *Create();

    // Protocols we can Start()
    static int ProtoLAT;
    static int ProtoMOP;
};

// Make sure we have the packet types
#ifndef ETHERTYPE_LAT
#define ETHERTYPE_LAT 0x6004
#endif

#ifndef ETHERTYPE_MOPRC
#define ETHERTYPE_MOPRC 0x6002
#endif

