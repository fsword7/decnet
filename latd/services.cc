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

#include <stdio.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <list>
#include <string>
#include <map>
#include <iterator>
#include <strstream>
#include <iomanip>

#include "lat.h"
#include "utils.h"
#include "services.h"
#include "dn_endian.h"

// Add or replace a node in the service table
bool LATServices::add_service(const string &node, const string &service, const string &ident,
			      int rating, int interface, unsigned char *macaddr)
{
    debuglog(("Got service. Node: %s, service %s, rating: %d\n",
	     node.c_str(), service.c_str(), rating));
    
    map<string, serviceinfo, less<string> >::iterator test = servicelist.find(service);
    if (test == servicelist.end())
    {
	servicelist[service] = serviceinfo(node, ident, macaddr, rating, interface);
    }
    else
    {
	servicelist[test->first].add_or_replace_node(node, ident, macaddr, rating, interface);
    }
    return true;
}

// Return the highest rated node providing a named service
bool LATServices::get_highest(const string &service, string &node, unsigned char *macaddr, 
			      int *interface)
{
  map<string, serviceinfo, less<string> >::iterator test = servicelist.find(service);
  if (test != servicelist.end())
  {
      return servicelist[test->first].get_highest(node, macaddr, interface);
  }
  return false; // Not found
}

// Return the highest rated node providing this service
bool LATServices::serviceinfo::get_highest(string &node, unsigned char *macaddr, int *interface)
{
  int                  highest_rating=0;
  string               highest_node;
  const unsigned char *highest_macaddr = NULL;
  
  map<string, nodeinfo, less<string> >::iterator i;
  for (i=nodes.begin(); i != nodes.end(); i++)
  {
      if (nodes[i->first].is_available() &&
	  nodes[i->first].get_rating() > highest_rating)
      {
	  highest_rating  = nodes[i->first].get_rating();
	  highest_node    = i->first; // Just note the pointer
	  highest_macaddr = nodes[i->first].get_macaddr();
	  *interface = nodes[i->first].get_interface();
      }
  }

  if (highest_macaddr)
  {
      // OK copy it now.
      memcpy(macaddr, highest_macaddr, 6);
      node = highest_node;
      return true;
  }
  else
  {
      return false;
  }
}


// Return the node macaddress if the node provides this service
bool LATServices::get_node(const string &service, const string &node, 
			   unsigned char *macaddr, int *interface)
{
  map<string, serviceinfo, less<string> >::iterator test = servicelist.find(service);
  if (test != servicelist.end())
  {
      return servicelist[test->first].get_node(node, macaddr, interface);
  }
  return false; // Not found
}


// Return the node's macaddress
bool LATServices::serviceinfo::get_node(const string &node, unsigned char *macaddr, int *interface)
{
  map<string, nodeinfo, less<string> >::iterator test = nodes.find(node);
  if (test != nodes.end())
  {
      memcpy(macaddr, nodes[node].get_macaddr(), 6);
      *interface = nodes[node].get_interface();
      return true;
  }
  else
  {
      return false;
  }
}


// Remove node from all services...
// Actually just mark as unavailable in all services
bool LATServices::remove_node(const string &node)
{
    bool removed = false;
    map<string, serviceinfo, less<string> >::iterator s(servicelist.begin());
    
    for (; s != servicelist.end(); s++)
    {
	if (servicelist[s->first].remove_node(node))
	    removed=true;	
    }
    return removed;
}


bool LATServices::serviceinfo::remove_node(const string &node)
{
    map<string, nodeinfo, less<string> >::iterator test = nodes.find(node);
    if (test != nodes.end())
    {
	nodes[test->first].set_available(false);	
    }
    return false;    
}

// Return true if the service is available. 
// This is the case if one node offering that service is available
bool LATServices::serviceinfo::is_available()
{    
    bool avail = true;
    map<string, nodeinfo, less<string> >::iterator n(nodes.begin());
    
    for (; n != nodes.end(); n++)
    {
	if (!nodes[n->first].is_available()) avail=false;
    }
    return avail;
}

// Verbose listing of nodes in this service
void LATServices::serviceinfo::list_service(ostrstream &output)
{
    map<string, nodeinfo, less<string> >::iterator n(nodes.begin());
    
    output << "Node Name        Status      Rating   Identification" << endl;
    for (; n != nodes.end(); n++)
    {
	output << setw(28 - n->first.length()) << n->first << 
	    (nodes[n->first].is_available()?"Reachable  ":"Unreachable") << "   " <<	  
	    setw(4) << nodes[n->first].get_rating() << "   " << 
	    nodes[n->first].get_ident() <<  endl;
    }
}

// List all known services
bool LATServices::list_services(bool verbose, ostrstream &output)
{
  map<string, serviceinfo, less<string> >::iterator s(servicelist.begin());

  for (; s != servicelist.end(); s++)
  {
      if (verbose)
      {
	  output << endl;
	  output << "Service Name:    " << s->first << endl;
	  output << "Service Status:  " << (servicelist[s->first].is_available()?"Available ":"Unavailable") << "   " << endl;
	  output << "Service Ident:   " << servicelist[s->first].get_ident() << endl << endl;
	  servicelist[s->first].list_service(output);
	  output << "--------------------------------------------------------------------------------" << endl;

      }
      else
      {
	  output << setw(28 - s->first.length()) << s->first << 
	      (servicelist[s->first].is_available()?"Available ":"Unavailable") << "   " <<
	      servicelist[s->first].get_ident() << endl;
      }

  }

  output << ends; // Trailing NUL for latcp's benefit.
  return true;
}

LATServices *LATServices::instance = NULL;


/* SERVICE LIST EXAMPLES (VMS):
/FULL:


Service Name:    BACON                    Service Type:  General
Service Status:  Available
Service Ident:   .Welcome to VAX/VMS V5.5    

Node Name            Status      Rating   Identification
BACON                On            83 D   .Welcome to VAX/VMS V5.5    

--------------------------------------------------------------------------------

Service Name:    BALTI            
Service Status:  Available
Service Ident:   Digital UNIX Version V4.0 LAT SERVICE

Node Name            Status      Rating   Identification
BALTI                Reachable    126     Digital UNIX Version V4.0 LAT SERVICE

--------------------------------------------------------------------------------

Service Name:    GROT                     Service Type:  General
Service Status:  Available
Service Ident:   Perrin

Node Name            Status      Rating   Identification
BACON                On           100 S   Perrin

--------------------------------------------------------------------------------

Service Name:    PATRIK           
Service Status:  Available
Service Ident:   Linux 2.2.14

Node Name            Status      Rating   Identification
PATRIK               Reachable      5     Linux 2.2.14
*/
