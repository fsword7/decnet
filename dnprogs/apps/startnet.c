/* (c) 1998 Eduardo Marcelo Serrat

   Modifications (c) 1998 by Patrick Caulfield to set the Ethernet address
                and  1999 to configure 2.3+ kernels
		and  2001 to set MAC address on all, or specified interfaces 
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <linux/sysctl.h>
#include <netdnet/dn.h>
#include <netdnet/dnetdb.h>
#include <netinet/in.h>
#include <features.h>    /* for the glibc version number */
#if (__GLIBC__ >= 2 && __GLIBC_MINOR >= 1) || __GLIBC__ >= 3
#include <netpacket/packet.h>
#include <net/ethernet.h>     /* the L2 protocols */
#else
#include <asm/types.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>   /* The L2 protocols */
#endif


static int set_hwaddr(int argc, char *argv[]);

struct 
{
    char	devname[5]; 
    char	exec_addr[6]; 
} if_arg;


int main(int argc, char *argv[])
{
  	int sockfd;
  	int er;
	unsigned char dn_hiord_addr[6] = {0xAA,0x00,0x04,0x00,0x00,0x00};

	char	*exec_dev;
	static  struct	dn_naddr	*binadr;

	if ((exec_dev=getexecdev()) == NULL)
	{
		printf("getexecdev: Invalid line in decnet.conf\n");
		exit (-1);
	}

	memcpy(&if_arg.devname,exec_dev,5);
	
	binadr=getnodeadd();
	if (binadr == NULL)
	{
		printf("dnet_addr: Invalid executor address in decnet.conf\n");
		exit (-1);
	}
	memcpy(&if_arg.exec_addr,dn_hiord_addr,6);
	if_arg.exec_addr[4]=binadr->a_addr[0];
	if_arg.exec_addr[5]=binadr->a_addr[1];


#ifdef SDF_UICPROXY 
#if 0 /* This doesn't really achieve anything */
	// Steve's Kernel uses syctl
	{
	    int name[] = {CTL_NET, NET_DECNET, NET_DECNET_DEFAULT_DEVICE};
	    int *oldlen;
	    char address[256];
	    int status;
	    struct nodeent *node;

	    strcpy(address, dnet_ntoa(binadr));
	    node = getnodebyaddr((char*)binadr->a_addr,2 , AF_DECnet);
	    if (!node)
	    {
		fprintf(stderr, "Can't get executor name from address %s\n",
			address);
		return -1;
	    }
	    
	    status = sysctl(name, 3, NULL, NULL,
			    exec_dev, strlen(exec_dev));
	    if (status) perror("sysctl(set exec dev)");

	    name[2] = NET_DECNET_NODE_ADDRESS;
	    status = sysctl(name, 3, NULL, NULL,
			    binadr->a_addr, 2);
	    if (status) perror("sysctl(set exec addr)");
    
	    name[2] = NET_DECNET_NODE_NAME;
	    status = sysctl(name, 3, NULL, NULL,
			    node->n_name, strlen(node->n_name));
	    if (status) perror("sysctl(set exec name)");
	}
#endif
#else
	// Eduardo's uses ioctl on an open socket
  	if ((sockfd=socket(AF_DECnet,SOCK_SEQPACKET,DNPROTO_NSP)) == -1) {
	    fprintf(stderr, "DECnet not supported in the kernel\n");
	    exit(-1);
  	}

	if ((er=ioctl(sockfd, SIOCSIFADDR, (unsigned long)&if_arg)) < 0) {
	    if (errno == EADDRINUSE)
		fprintf(stderr, "DECnet is already running\n");
	    else
		perror("ioctl");
	    close(sockfd);
	    exit(-1);
	}		
	
  	if ( close(sockfd) < 0)
	    perror("close");
#endif

        // Setting the hardware address is common to both
	if (argc > 1 && !strcmp(argv[1], "-hw"))
	{
	    return set_hwaddr(argc-2, argv+2);
	}
	return 0;
}


/* See if the current interface is one we need to change */
static int use_if(char *name, int argc, char *argv[])
{
    int i;
    
    if (argc == 0) return 1; /* Do em all */

    for (i=0; i<argc; i++)
	if (strcmp(name, argv[i]) == 0) return 1;

    return 0;
}

/* Change the MAC(hardware) address on specified(or all) ethernet
   interfaces */
static int set_hwaddr(int argc, char *argv[])
{
    struct ifreq ifr;
    int iindex = 1;
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //int sock = socket(AF_DECnet, SOCK_STREAM, DNPROTO_NSP);
    int ifdone = 0;
    int ret = 0;
    int force = 0;

    /* Don't down the interface unless we have to. */
    if (argc > 1 && strcmp(argv[0], "-f")==0)
	force = 1;

    ifr.ifr_ifindex = iindex;

    while (ioctl(sock, SIOCGIFNAME, &ifr) == 0)
    {
	/* Only use ethernet interfaces */
	ioctl(sock, SIOCGIFHWADDR, &ifr);
	if (ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER)
	{
	    if (use_if(ifr.ifr_name, argc, argv))
	    {
		/* Down the interface so we can change the MAC address */		
		ioctl(sock, SIOCGIFFLAGS, &ifr);
		if (ifr.ifr_flags & IFF_UP && force)
		{
		    ifr.ifr_flags &= ~IFF_UP;
		    ioctl(sock, SIOCSIFFLAGS, &ifr);
		}

		/* Need to refresh this */
		ifr.ifr_ifindex = iindex;				
		ioctl(sock, SIOCGIFHWADDR, &ifr);
		memcpy(ifr.ifr_hwaddr.sa_data, if_arg.exec_addr, 6);

		/* Do the deed */
		if (ioctl(sock, SIOCSIFHWADDR, &ifr) < 0)
		{
		    fprintf(stderr, "Error setting hw address on %s: %s\n",
			    ifr.ifr_name, strerror(errno));
		    ret = errno;		
		}

		/* "UP" the interface. Just in case TCP/IP is not running */
		ioctl(sock, SIOCGIFFLAGS, &ifr);		    
		ifr.ifr_flags |= IFF_UP;
		ioctl(sock, SIOCSIFFLAGS, &ifr);
		    
		ifdone++;
	    }
	}	    
	ifr.ifr_ifindex = ++iindex;
    }

    /* If interfaces were specified and none were done then
       that's an error */
    if (!ifdone && argc)
    {
	fprintf(stderr, "No interfaces set for DECnet\n");
	ret = -1;
    }
    
    close(sock);
    return ret;
}
