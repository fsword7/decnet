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
    bool get_highest(const string &service,
		     string &node,
		     unsigned char *macaddr);

    bool get_node(const string &service,
		  const string &node,
		  unsigned char *macaddr);

    // Add/update a service.
    bool add_service(const string &node, const string &service, const string &ident,
		     int rating, unsigned char *macaddr);

    bool remove_node(const string &node);
    bool list_services(bool verbose, ostrstream &output);
    void purge() {servicelist.clear(); }
    
 private:
    LATServices()
      {};                         // Private constructor to force singleton
    static LATServices *instance; // Singleton instance

    class serviceinfo
    {
    public:
      serviceinfo() {}
      serviceinfo(const string &node, const string &_ident,
		  const unsigned char *macaddr, int rating)
	  {
	      ident = _ident;
	      nodes[node] = nodeinfo(macaddr, rating, ident);
	  }
      
      void add_or_replace_node(const string &node, const string &_ident,
			       const unsigned char *macaddr, int rating)
	  {
	      ident = _ident;
	      nodes.erase(node); // Don't care if this fails
	      nodes[node] = nodeinfo(macaddr, rating, ident);
	  }
      bool         get_highest(string &node, unsigned char *macaddr);
      bool         get_node(const string &node, unsigned char *macaddr);
      const string get_ident() { return ident; }
      bool         is_available();
      bool         remove_node(const string &node);
      void         serviceinfo::list_service(ostrstream &output);

    private:      
      class nodeinfo
	{
	public:
	  nodeinfo() {}
	  nodeinfo(const unsigned char *_macaddr, int _rating, string _ident):
	      rating(_rating),
	      available(true)
	    {
	      memcpy(macaddr, _macaddr, 6);
	      ident = _ident;
	    }
	  int                  get_rating()          { return rating; }
	  const unsigned char *get_macaddr()         { return macaddr; }
	  bool                 is_available()        { return available; }
	  void                 set_available(bool a) { available = a; }
	  string               get_ident()           { return ident; }
	  
	private:
	  unsigned char macaddr[6];
	  int           rating;
	  bool          available;
	  string        ident;
	};// class LATServices::service::nodeinfo

      map<string, nodeinfo, less<string> > nodes;
      string ident;
    };// class LATServices::serviceinfo
    
    map<string, serviceinfo, less<string> > servicelist;
};
