/* (c) 1998 Eduardo Marcelo Serrat

   Modifications (c) 1998 by Patrick Caulfield to set the Ethernet address
                and  1999 to configure 2.3+ kernels
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
#include <net/if.h>


static int set_hwaddr(void);

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
	    return set_hwaddr();
	}
	return 0;
}


int set_hwaddr(void)
{
    struct ifreq ifr;
    int skfd;
    
    skfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (skfd == -1) return -1;
    
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, if_arg.devname);
    ifr.ifr_hwaddr.sa_family  = AF_UNIX;
    memcpy(ifr.ifr_hwaddr.sa_data, if_arg.exec_addr, 6);

    if (ioctl(skfd, SIOCSIFHWADDR, &ifr) < 0) {
	perror("error setting hw address");
	return -1;
    }

    close(skfd);
    return 0;
}
