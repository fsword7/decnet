/******************************************************************************
    (c) 1995-1998 E.M. Serrat          emserrat@geocities.com
    (c) 1999      P.J. Caulfield       patrick@tykepenguin.cix.co.uk

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <netdnet/dn.h>
#include <netdnet/dnetdb.h>

static char             nodetag[80],nametag[80],nodeadr[80],nodename[80];
static struct nodeent	dp;
static struct dn_naddr	*naddr;

struct nodeent *getnodebyname(char *name)
{
	FILE		*dnhosts;
	char		nodeln[80];
	int             a,n;

	/* See if it is an address really */
	if (sscanf(name, "%d.%d", &a, &n) == 2)
	{
	    static struct dn_naddr addr;
	    addr.a_addr[0] = n & 0xFF;
	    addr.a_addr[1] = (a << 2) | ((n & 0x300) >> 8);
	    dp.n_addr = (char *)&addr.a_addr;
	    dp.n_length=2;
	    dp.n_name=(unsigned char *)name;  /* No point looking this up for a real name */
	    dp.n_addrtype=AF_DECnet;

	    return &dp;
	}

	if ((dnhosts = fopen(SYSCONF_PREFIX "/etc/decnet.conf","r")) == NULL)
	{
		printf("getnodebyname: Can not open " SYSCONF_PREFIX "/etc/decnet.conf\n");
		return 0;
	}
	while (fgets(nodeln,80,dnhosts) != NULL)
	{
		sscanf(nodeln,"%s%s%s%s\n",nodetag,nodeadr,nametag,nodename);
		if (strncmp(nodetag,"#",1) != 0)	
		{
		   if (((strcmp(nodetag,"executor") != 0) &&
	    	       (strcmp(nodetag,"node")     != 0)) ||
		       (strcmp(nametag,"name")     != 0))
		   {
		       printf("getnodebyname: Invalid decnet.conf syntax\n");
			 fclose(dnhosts);
		       return 0;
		   }
		   if (strcmp(nodename,name) == 0) 
		   {
			fclose(dnhosts);
			if ( (naddr=dnet_addr(nodename)) == 0)
					return 0;
			dp.n_addr=(unsigned char *)&naddr->a_addr; 
			dp.n_length=2;
			dp.n_name=nodename;		
			dp.n_addrtype=AF_DECnet;
			return &dp;
		   }
		}
	}
	fclose(dnhosts);
	return 0;
}
/*--------------------------------------------------------------------------*/
