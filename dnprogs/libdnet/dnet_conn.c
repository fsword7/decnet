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
static int              consock;

int dnet_conn(char *host, char *objname, int type, unsigned char *opt_out, int opt_outl, unsigned char *opt_in, int opt_inl)
{
	FILE		*dnhosts;
	char		nodeln[80];
	struct	sockaddr_dn		sockaddr;
	static  struct	dn_naddr	*binaddr;

	if ((dnhosts = fopen(SYSCONF_PREFIX "/etc/decnet.conf","r")) == NULL)
	{
		printf("dnet_conn: Can not open " SYSCONF_PREFIX "/etc/decnet.conf\n");
		return -1;
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
		       printf("dnet_conn: Invalid decnet.conf syntax\n");
		       return -1;
		   }
		   if (strcmp(nodename,host) == 0) 
		   {
			fclose(dnhosts);
			binaddr = dnet_addr(nodename);
			if (binaddr == NULL) return -1;
  			if ((consock=socket(AF_DECnet,SOCK_SEQPACKET,
				           DNPROTO_NSP)) < 0) 
	    		return -1;


			sockaddr.sdn_family = AF_DECnet;
			sockaddr.sdn_flags	= 0x01;
			sockaddr.sdn_objnum	= 0x00;		
			sockaddr.sdn_objnamel	= strlen(objname);
			memcpy(sockaddr.sdn_objname,objname,strlen(objname));
			memcpy(sockaddr.sdn_add.a_addr, binaddr->a_addr,2);

			if (connect(consock, (struct sockaddr *)&sockaddr, 
				sizeof(sockaddr)) < 0) return -1;
			else	return consock;
		   }
		}
	}
	fclose(dnhosts);
	return -1;
}
/*--------------------------------------------------------------------------*/
