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

class LATServices
{
 public:
    static LATServices *Instance()
	{
	    if (!instance)
		return (instance = new LATServices());
	    else
		return instance;
	}
    
    // Get the highest rated node for this service name.
    bool get_highest(const std::string &service,
		     std::string &node,
		     unsigned char *macaddr,
		     int *interface);

    bool get_node(const std::string &service,
		  const std::string &node,
		  unsigned char *macaddr, int *interface);

    // Add/update a service.
    bool add_service(const std::string &node, const std::string &service, const std::string &ident,
		     int rating, int interface, unsigned char *macaddr);

    bool remove_node(const std::string &node);
    bool list_services(bool verbose, std::ostrstream &output);
    void purge() {servicelist.clear(); }
    
 private:
    LATServices()
      {};                         // Private constructor to force singleton
    static LATServices *instance; // Singleton instance

    class serviceinfo
    {
    public:
      serviceinfo() {}
      serviceinfo(const std::string &node, const std::string &_ident,
		  const unsigned char *macaddr, int rating, int interface)
	  {
	      ident = _ident;
	      nodes[node] = nodeinfo(macaddr, rating, ident, interface);
	  }
      
      void add_or_replace_node(const std::string &node, const std::string &_ident,
			       const unsigned char *macaddr, int rating,
			       int interface)
	  {
	      ident = _ident;
	      nodes.erase(node); // Don't care if this fails
	      nodes[node] = nodeinfo(macaddr, rating, ident, interface);
	  }
      bool         get_highest(std::string &node, unsigned char *macaddr, int *interface);
      bool         get_node(const std::string &node, unsigned char *macaddr, int *interface);
      const std::string get_ident() { return ident; }
      bool         is_available();
      bool         remove_node(const std::string &node);
      void         serviceinfo::list_service(std::ostrstream &output);

    private:      
      class nodeinfo
	{
	public:
	  nodeinfo() {}
	  nodeinfo(const unsigned char *_macaddr, int _rating, std::string _ident, int _interface):
	      rating(_rating),
	      interface(_interface),
	      available(true)
	    {
	      memcpy(macaddr, _macaddr, 6);
	      ident = _ident;
	    }
	  int                  get_rating()          { return rating; }
	  int                  get_interface()       { return interface; }
	  const unsigned char *get_macaddr()         { return macaddr; }
	  bool                 is_available()        { return available; }
	  void                 set_available(bool a) { available = a; }
	  std::string          get_ident()           { return ident; }
	  
	private:
	  unsigned char macaddr[6];
	  int           rating;
	  int           interface;
	  bool          available;
	  std::string   ident;
	};// class LATServices::service::nodeinfo

      std::map<std::string, nodeinfo, std::less<std::string> > nodes;
      std::string ident;
    };// class LATServices::serviceinfo
    
    std::map<std::string, serviceinfo, std::less<std::string> > servicelist;
};
