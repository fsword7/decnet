/******************************************************************************
    (c) 1995-1998 E.M. Serrat          emserrat@geocities.com

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
static char             asc_addr[6];
static struct nodeent	dp;
static char   		laddr[2];

struct nodeent *getnodebyaddr(const char *addr, int len, int family)
{
	FILE	*dnhosts;
	char	nodeln[80];

	sprintf (asc_addr,"%d.%d",((char)*(addr+1) >> 2),
		 (((char)*(addr+1) & 0x03) << 8) | ((char)*(addr))
		);
	
	if ((dnhosts = fopen(SYSCONF_PREFIX "/etc/decnet.conf","r")) == NULL)
	{
		printf("getnodebyaddr: Can not open " SYSCONF_PREFIX "/etc/decnet.conf\n");
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
		       printf("getnodebyaddr: Invalid decnet.conf syntax\n");
			 fclose(dnhosts);
		       return 0;
		   }
		   if (strcmp(nodeadr,asc_addr) == 0) 
		   {
			memcpy(laddr,addr,len);
			dp.n_addr=(unsigned char *)&laddr;
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

