/*
 * YOU SHOULD NOT BE USING THIS FILE!
 *
 * This is a sample dn.h file distributed with the dnprogs programs to
 * get them to compile in dire emergencies. Programs compiled using this
 * include file may work, they may not. If they do not then
 * DON'T BLAME ME!
 *
 * You should patch your kernel with the patches from
 * http://linux.dreamtime.org/decnet  or
 * http://www.sucs.swan.ac.uk/~rohan/DECnet
 * and then build the programs. That way we can support you.
*/

#ifndef	NETDNET_DN_H
#define NETDNET_DN_H

#warning Not using kernel provided dn.h file.

// This is how desperate we are.
#ifndef AF_DECnet
#define AF_DECnet 12
#endif

#define DNPROTO_NSP	2 /* This can't be the same as SOL_SOCKET */
#define SO_LINKINFO	7
#define DN_ADDL		2
#define DN_MAXADDL	2
#define DN_MAXOPTL	16
#define DN_MAXOBJL	16
#define DN_MAXACCL	40
#define DN_MAXALIASL	128
#define DN_MAXNODEL	256

/* SET/GET Socket options */
#define SO_CONDATA	1
#define SO_CONACCESS	2
#define SO_PROXYUSR	3

/* LINK States */
#define LL_INACTIVE	0
#define LL_CONNECTING	1
#define LL_RUNNING	2
#define LL_DISCONNECTING 3

/* Structures */
struct dn_naddr 
{
	unsigned short		a_len;
	unsigned char a_addr[DN_MAXADDL];
};

struct sockaddr_dn
{
	unsigned short		sdn_family;
	unsigned char		sdn_flags;
	unsigned char		sdn_objnum;
	unsigned short		sdn_objnamel;
	unsigned char		sdn_objname[DN_MAXOBJL];
#define sdn_nodeaddrl	sdn_add.a_len		/* X11R6.4 compatibility */
	struct   dn_naddr	sdn_add;
};

struct optdata_dn
{
	unsigned char		opt_sts;
	unsigned char 		opt_optl;
	unsigned char		opt_data[DN_MAXOPTL];
};

struct accessdata_dn
{
	unsigned char		acc_accl;
	unsigned char		acc_acc[DN_MAXACCL];
	unsigned char 		acc_passl;
	unsigned char		acc_pass[DN_MAXACCL];
	unsigned char 		acc_userl;
	unsigned char		acc_user[DN_MAXACCL];
};

#endif				/* NETDNET_DN_H */
