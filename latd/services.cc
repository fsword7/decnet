/******************************************************************************
    (c) 2000-2004 Christine Caulfield                 christine.caulfield@googlemail.com
    (c) 2003 Dmitri Popov

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
#include <stdio.h>
#include <time.h>
#include <netinet/in.h>
#include <string.h>
#include <list>
#include <string>
#include <map>
#include <iterator>
#include <sstream>
#include <iomanip>

#include "lat.h"
#include "utils.h"
#include "services.h"
#include "dn_endian.h"

// Add or replace a node in the service table
bool LATServices::add_service(const std::string &node, const std::string &service, const std::string &ident,
			      int rating, int interface, unsigned char *macaddr)
{
    debuglog(("Got service. Node: %s, service %s, rating: %d\n",
	     node.c_str(), service.c_str(), rating));

    std::map<std::string, serviceinfo, std::less<std::string> >::iterator test = servicelist.find(service);
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
bool LATServices::get_highest(const std::string &service, std::string &node, unsigned char *macaddr,
			      int *interface)
{
  std::map<std::string, serviceinfo, std::less<std::string> >::iterator test = servicelist.find(service);
  if (test != servicelist.end())
  {
      return servicelist[test->first].get_highest(node, macaddr, interface);
  }
  return false; // Not found
}

// Return the highest rated node providing this service
bool LATServices::serviceinfo::get_highest(std::string &node, unsigned char *macaddr, int *interface)
{
  int                  highest_rating=0;
  std::string          highest_node;
  const unsigned char *highest_macaddr = NULL;

  std::map<std::string, nodeinfo, std::less<std::string> >::iterator i;
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
bool LATServices::get_node(const std::string &service, const std::string &node,
			   unsigned char *macaddr, int *interface)
{
  std::map<std::string, serviceinfo, std::less<std::string> >::iterator test = servicelist.find(service);
    std::map<std::string, serviceinfo, std::less<std::string> >::iterator c =
servicelist.begin();
    int i =0;
     for(;c != servicelist.end();c++) {
       i++;
debuglog(("LATServices::get_node : service %2d '%s' \n",
  i, c->first.c_str()));
}

  if (test != servicelist.end())
  {
//      std::map<std::string, serviceinfo, std::less<std::string> >::iterator c = test;
//	  int i =0;
//	   for(;c != servicelist.end();c++) { i++; };
debuglog(("LATServices::get_node : service found '%s', number: %d \n", service.c_str(), i));
      return servicelist[test->first].get_node(node, macaddr, interface);
  }
debuglog(("LATServices::get_node : no service '%s' \n", service.c_str()));

  return false; // Not found
}


// Return the node's macaddress
bool LATServices::serviceinfo::get_node(const std::string &node, unsigned char *macaddr, int *interface)
{
  std::map<std::string, nodeinfo, std::less<std::string> >::iterator test = nodes.find(node);

   std::map<std::string, nodeinfo, std::less<std::string> >::iterator c =
nodes.begin();
    int i =0;
     for(;c != nodes.end();c++) {
       i++;
debuglog(("LATServices::serinfo::get_node :node %2d '%s' \n",
  i, c->first.c_str()));
}

debuglog(("LATServices::serinfo::get_node looking for '%s' \n",
  node.c_str()));


  if (test != nodes.end())
  {
      memcpy(macaddr, nodes[node].get_macaddr(), 6);
      *interface = nodes[node].get_interface();
      return true;
  }
  else
  {
debuglog(("LATServices::serviceinfo::get_node : no node '%s' \n", node.c_str()));
      return false;
  }
}


// Remove node from all services...
// Actually just mark as unavailable in all services
bool LATServices::remove_node(const std::string &node)
{
    bool removed = false;
    std::map<std::string, serviceinfo, std::less<std::string> >::iterator s(servicelist.begin());

    for (; s != servicelist.end(); s++)
    {
	if (servicelist[s->first].remove_node(node))
	    removed=true;
    }
    return removed;
}


bool LATServices::serviceinfo::remove_node(const std::string &node)
{
    std::map<std::string, nodeinfo, std::less<std::string> >::iterator test = nodes.find(node);
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
    std::map<std::string, nodeinfo, std::less<std::string> >::iterator n(nodes.begin());

    for (; n != nodes.end(); n++)
    {
	if (!nodes[n->first].is_available()) avail=false;
    }
    return avail;
}


void LATServices::serviceinfo::expire_nodes(time_t current_time)
{
    std::map<std::string, nodeinfo, std::less<std::string> >::iterator n(nodes.begin());

    for (; n != nodes.end(); n++)
    {
	if (nodes[n->first].has_expired(current_time))
	    nodes[n->first].set_available(false);
    }
}

void LATServices::expire_nodes()
{
    std::map<std::string, serviceinfo, std::less<std::string> >::iterator s(servicelist.begin());
    time_t current_time = time(NULL);

    for (; s != servicelist.end(); s++)
    {
	// Skip dummy service
	if (s->first != "")
	{
	    servicelist[s->first].expire_nodes(current_time);
	}
    }
}


// Verbose listing of nodes in this service
void LATServices::serviceinfo::list_service(std::ostringstream &output)
{
    std::map<std::string, nodeinfo, std::less<std::string> >::iterator n(nodes.begin());

    output << "Node Name        Status      Rating   Identification" << std::endl;
    for (; n != nodes.end(); n++)
    {
        output.width(17);
        output.setf(std::ios::left, std::ios::adjustfield);
	output << n->first.c_str() <<
	         (nodes[n->first].is_available()?"Reachable  ":"Unreachable") << "   ";
        output.width(4);
        output.setf(std::ios::right, std::ios::adjustfield);
	output << nodes[n->first].get_rating() << "   " <<
 	          nodes[n->first].get_ident() <<  std::endl;
    }
}

void LATServices::serviceinfo::list_nodes(std::ostringstream &output)
{
    std::map<std::string, nodeinfo, std::less<std::string> >::iterator n(nodes.begin());
    const unsigned char *addr = NULL;

    output << "Node Name        Status       Address            Identification" << std::endl;
    for (; n != nodes.end(); n++)
    {
        addr = n->second.get_macaddr();
        output.width(17);
        output.setf(std::ios::left, std::ios::adjustfield);
        output << n->first.c_str() <<
            (n->second.check_respond_counter()?"Reachable  ":"Unreachable") << "  ";

        output.setf(std::ios::hex, std::ios::basefield);

        output << setiosflags(std::ios::right | std::ios::uppercase) << std::setfill('0')
           << std::setw(2) << (int)addr[0] << '-'
           << std::setw(2) << (int)addr[1] << '-'
           << std::setw(2) << (int)addr[2] << '-'
           << std::setw(2) << (int)addr[3] << '-'
           << std::setw(2) << (int)addr[4] << '-'
           << std::setw(2) << (int)addr[5]
           << resetiosflags(std::ios::right | std::ios::uppercase) << std::setfill(' ');

        output.setf(std::ios::right, std::ios::adjustfield);
        output << "  " << n->second.get_ident() <<  std::endl;
    }
}

bool LATServices::list_dummy_nodes(bool verbose, std::ostringstream &output)
{
    std::map<std::string, serviceinfo, std::less<std::string> >::iterator dummies =
             servicelist.find("");

    if ( dummies == servicelist.end()) {
        output << "No dummy nodes available." << std::endl;
        return true;
    }

    output << std::endl;
    output << "Service Name:    " << "Slave nodes" << std::endl;
    output << "Service Status:  " << (servicelist[dummies->first].is_available()?"Available ":"Unavailable") << "   " << std::endl;
    output << "Service Ident:   " << servicelist[dummies->first].get_ident() << std::endl << std::endl;
    dummies->second.list_nodes(output);
    output << "--------------------------------------------------------------------------------" << std::endl;


    return true;
}

bool LATServices::touch_dummy_node_respond_counter(const std::string &str_name)
{
    std::map<std::string, serviceinfo, std::less<std::string> >::iterator dummies =
             servicelist.find("");

    if ( dummies == servicelist.end()) {
        return false; // no node
    }

    return dummies->second.touch_dummy_node_respond_counter(str_name);
}

bool LATServices::serviceinfo::touch_dummy_node_respond_counter(const std::string &str_name)
{
    std::map<std::string, nodeinfo, std::less<std::string> >::iterator n = nodes.find(str_name);

    if (n == nodes.end()) {
        return false; // no node
    }

    debuglog(("touch_respond() : node: %s ", n->first.c_str()));
    n->second.touch_respond_counter();
    return true;
}


// List all known services
bool LATServices::list_services(bool verbose, std::ostringstream &output)
{
  std::map<std::string, serviceinfo, std::less<std::string> >::iterator s(servicelist.begin());

  for (; s != servicelist.end(); s++)
  {
      // Skip dummy service
      if (s->first != "")
      {
	  if (verbose)
	  {
	      output << std::endl;
	      output << "Service Name:    " << s->first << std::endl;
	      output << "Service Status:  " << (servicelist[s->first].is_available()?"Available ":"Unavailable") << "   " << std::endl;
	      output << "Service Ident:   " << servicelist[s->first].get_ident() << std::endl << std::endl;
	      servicelist[s->first].list_service(output);
	      output << "--------------------------------------------------------------------------------" << std::endl;

	  }
	  else
	  {
              output.width(28);
              output.setf(std::ios::left, std::ios::adjustfield);
	      output << s->first.c_str() << (servicelist[s->first].is_available()?"Available ":"Unavailable") << "   " <<
                        servicelist[s->first].get_ident() << std::endl;
	  }
      }

  }

  output << std::ends; // Trailing NUL for latcp's benefit.
  return true;
}

LATServices *LATServices::instance = NULL;
