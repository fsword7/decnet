/******************************************************************************
    (c) 1998-2000 P.J. Caulfield               patrick@pandh.demon.co.uk
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 ******************************************************************************
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <syslog.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
// Horrible hack for glibc 2.1 which defines getnodebyname
#if __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 1
#define getnodebyname ipv6_getnodebyname
#endif

#include <netdb.h>

#if __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 1
#undef getnodebyname 
#endif

#include <netdnet/dnetdb.h>
#include <netdnet/dn.h>
#include <netinet/in.h>
#include <uudeview.h>
#include "configfile.h"

#ifndef SENDMAIL_COMMAND
#define SENDMAIL_COMMAND "/usr/sbin/sendmail -oem"
#endif

char response[1024];
int send_smtp(int sock,
	      char *addressees,
	      char *remote_hostname,
	      char *remote_user,
	      char *subject,
	      char *full_user
	      );

int send_smail(int sock,
	       char *addressees,
	       char *remote_hostname,
	       char *remote_user,
	       char *subject,
	       char *full_user
	      );
int send_body(int dnsock, FILE *unixfile);

int smtp_response(FILE *sockstream);
int smtp_error(FILE *sockstream);


extern int verbosity;
static int is_binary = 0;

// Receive a message from VMS and pipe it into sendmail
void receive_mail(int sock)
{
    char   remote_user[256]; // VMS only sends 12 but...just in case!
    char   local_user[256];
    char   addressees[65536];
    char   full_user[256];
    char   subject[256];
    char   remote_hostname[256];
    struct sockaddr_dn sockaddr;
    struct optdata_dn  optdata;
    int    num_addressees=0;
    int    i;
    int    stat;
    int    namlen = sizeof(sockaddr);
    int    optlen = sizeof(optdata);

    // If the CONDATA structure contains 16 bytes then we are being sent binary
    // data using MAIL/FOREIGN
    getsockopt(sock, DNPROTO_NSP, SO_CONDATA, &optdata, &optlen);
    if (optdata.opt_optl == 0x10)
	is_binary = 1;
    
    // Get the remote host name
    stat = getpeername(sock, (struct sockaddr *)&sockaddr, &namlen);
    if (!stat)
    {
        strcpy(remote_hostname, dnet_htoa(&sockaddr.sdn_add));
    }
    else
    {
	sprintf(remote_hostname, "%d.%d",
		(sockaddr.sdn_add.a_addr[1] >> 2),
		(((sockaddr.sdn_add.a_addr[1] & 0x03) << 8) |
		 sockaddr.sdn_add.a_addr[0]));
    }
    
    // Read the remote user name - this comes in padded with spaces to
    // 12 characters
    stat = read(sock, remote_user, sizeof(remote_user)); 
    if (stat < 0)
    {
	DNETLOG((LOG_ERR, "Error reading remote user: %m"));
	return;
    }

    // Trim the remote user.
    remote_user[stat] = '\0'; // Just in case its the full buffer
    i=stat;
    while (remote_user[i] == ' ') remote_user[i--] = '\0';
    
    // The rest of the message should be the local user names. These could
    // be a real local user or an internet mail name. They are not
    // padded.
    addressees[0] = '\0';
    do
    {
	stat = read(sock, local_user, sizeof(local_user));
	if (stat == -1)
	{
	    DNETLOG((LOG_ERR, "Error reading local user: %m"));
	    return;
	}
	if (local_user[0] != '\0')
	{
	    local_user[stat] = '\0';
	    DNETLOG((LOG_DEBUG, "got local user: %s\n", local_user));
	    strcat(addressees, local_user);
	    strcat(addressees, ",");
	    
	    // Send acknowledge
	    write(sock, "\001\000\000\000", 4);
	    num_addressees++;
	}
    }
    while (local_user[0] != '\0');

    // Remove trailing comma
    addressees[strlen(addressees)-1] = '\0';

    // TODO: This should be more intelligent and only lower-case the
    // addressable part of the email name.
    for (i=0; i<strlen(addressees); i++)
	addressees[i] = tolower(addressees[i]);
    
    // This is the collected list of users to send the message to,
    // as entered by the user.
    // We enter it in an extension header
    stat = read(sock, full_user, sizeof(full_user));
    if (stat == -1)
    {
	DNETLOG((LOG_ERR, "Error reading full_user: %m"));
	return;
    }
    full_user[stat] = '\0';
    
    // Get the subject
    stat = read(sock, subject, sizeof(subject));
    if (stat == -1)
    {
	DNETLOG((LOG_ERR, "Error reading subject: %m"));
	return;
    }
    subject[stat] = '\0';

    DNETLOG((LOG_INFO, "Forwarding mail from %s::%s to %s\n",
	     remote_hostname, remote_user, addressees));

// Decide on sendmail or SMTP

    if (config_smtphost[0] == '\0')
	stat = send_smail(sock, addressees, remote_hostname, remote_user,
			  subject, full_user);
    else
	stat = send_smtp(sock, addressees, remote_hostname, remote_user,
			 subject, full_user);
	
    if (stat == 0)
    {
	// Send acknowledge - one for each addressee
	for (i=0; i<num_addressees; i++)
	{
	    write(sock, "\001\000\000\000", 4);
	}
    }
    close(sock);
}

int send_smtp(int sock,
	      char *addressees,
	      char *remote_hostname,
	      char *remote_user,
	      char *subject,
	      char *full_user
	      )
{
    FILE              *sockfile;
    struct sockaddr_in serv_addr;
    struct hostent    *host_info;
    struct servent    *servent;
    int                port;
    char              *addr;
    int                stat;
    int                numbytes;
    int                smtpsock;

    smtpsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (smtpsock == -1)
    {
	DNETLOG((LOG_ERR, "Can't open socket for SMTP: %m"));
	return -1;
    }
    
    host_info = gethostbyname(config_smtphost);
    if (host_info == NULL)
    {
        DNETLOG((LOG_ERR, "Cannot resolve host name: %s\n", config_smtphost));
        return -1;
    }
    
    servent = getservbyname("smtp", "tcp");
    if (servent != NULL)
    {
	port = ntohs(servent->s_port);
    }
    if (port == 0) port = 25;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    serv_addr.sin_addr.s_addr = *(int*)host_info->h_addr_list[0];
    
    if (connect(smtpsock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    {
	DNETLOG((LOG_ERR, "Cannot connect to SMTP server: %m"));
	return -1;
    }

/* Send initial SMTP commands and swallow responses */
    sockfile = fdopen(smtpsock, "a+");
    if (smtp_response(sockfile) != 220) return smtp_error(sockfile);

	
    fprintf(sockfile, "HELO %s\n", config_hostname);
    stat = smtp_response(sockfile);
    if (stat == 220)
	stat = smtp_response(sockfile);
    if (stat != 250) return smtp_error(sockfile);
    
    fprintf(sockfile, "MAIL FROM: <%s@%s>\n",
	    config_vmsmailuser, config_hostname);
    if (smtp_response(sockfile) != 250) return smtp_error(sockfile);

    addr = strtok(addressees, ",");
    while(addr)
    {
	fprintf(sockfile, "RCPT TO:<%s>\n", addr);
	if (smtp_response(sockfile) != 250) return smtp_error(sockfile);
	
	addr = strtok(NULL, ",");
    }
    
    fprintf(sockfile, "DATA\n");
    if (smtp_response(sockfile) != 354) return smtp_error(sockfile);

    // Send the header
    fprintf(sockfile, "From: %s@%s (%s::%s)\n",
	    config_vmsmailuser, config_hostname, remote_hostname,
	    remote_user);
    fprintf(sockfile, "Subject: %s\n", subject);
    fprintf(sockfile, "To: %s\n", addressees);
    if (is_binary)
    {
	fprintf(sockfile, "Mime-Version: 1.0\n");
	fprintf(sockfile, "Content-Type: application/octet-stream\n");
	fprintf(sockfile, "Content-Transfer-Encoding: base64\n");
    }
    fprintf(sockfile, "X-VMSmail: %s\n", full_user);
    
    fprintf(sockfile, "\n");
    
    send_body(sock, sockfile);

    fprintf(sockfile, ".\n");
    if (smtp_response(sockfile) != 250) return smtp_error(sockfile);
    
    fprintf(sockfile, "QUIT\n");
    if (smtp_response(sockfile) != 221) return smtp_error(sockfile);

    fclose(sockfile);
    return 0;
}


int send_smail(int sock,
	       char *addressees,
	       char *remote_hostname,
	       char *remote_user,
	       char *subject,
	       char *full_user
    )
{
    char   buf[65536];
    FILE  *mailpipe;
    int    stat;
     
    // Open a pipe to sendmail.
    sprintf(buf, "%s '%s'" , SENDMAIL_COMMAND, addressees);
    mailpipe = popen(buf, "w");
    if (mailpipe != NULL)
    {
	// Send the header
	fprintf(mailpipe, "From: %s@%s (%s::%s)\n",
		config_vmsmailuser, config_hostname, remote_hostname,
		remote_user);
	fprintf(mailpipe, "Subject: %s\n", subject);
	fprintf(mailpipe, "To: %s\n", addressees);
	if (is_binary)
	{
	    fprintf(mailpipe, "Mime-Version: 1.0\n");
	    fprintf(mailpipe, "Content-Type: application/octet-stream\n");
	    fprintf(mailpipe, "Content-Transfer-Encoding: base64\n");
	}
	fprintf(mailpipe, "X-VMSmail: %s\n", full_user);

	fprintf(mailpipe, "\n");

	send_body(sock, mailpipe);
	
	pclose(mailpipe);
    }
    else
    {
	DNETLOG((LOG_ERR, "Can't open pipe to sendmail: %m"));
	return -1;
    }
    return 0;
}


/*
  Read a response from the MTA and return the code number
  and the text of the response.
 */
int smtp_response(FILE *sockstream)
{
    int   status;
    int   len;
    char  codestring[5];

    if (fgets(response, sizeof(response), sockstream) == NULL)
    {
	strcpy(response, strerror(errno));
	return 0;
    }
    
    strncpy(codestring, response, 3);
    codestring[3] = '\0';
    status = atoi(codestring);

    return status;
}

int smtp_error(FILE *sockstream)
{
    DNETLOG((LOG_ERR, "SMTP Error: %s\n", response));
    fclose(sockstream);
    return -1;
}

// Sends the message body, converting binary messages to MIME if necessary
int send_body(int dnsock, FILE *unixfile)
{
    char  buf[65535];
    int   stat;
    int   finished = 0;
    
    // Get the text of the message
    if (!is_binary)
    {
	do
	{
	    stat = read(dnsock, buf, sizeof(buf));
	    if (stat == -1 && errno != ENOTCONN)
	    {
		DNETLOG((LOG_ERR, "Error reading message text: %m"));
		return -1;
	    }
	    
	    // VMS sends a NUL as the end of the message
	    if (buf[stat-1] == '\0') finished = 1;
	    if (stat > 0)
	    {
		buf[stat] = '\0';
		fprintf(unixfile, "%s\n", buf);
	    }
	}
	while (stat > 0 && !finished);
    }
    else
    {
	int len;
	char tempname[64];
	FILE *t;

	sprintf(tempname, "/tmp/vmsmail-%d", getpid());
	t = fopen(tempname, "w+");
	while ( (len=read(dnsock, buf, sizeof(buf))) > 1)
	    fwrite(buf, len, 1, t);
	fclose (t);

	// Base64 encode it.
	UUInitialize();
	UUEncodeToStream(unixfile, NULL, tempname, B64ENCODED, "temp.bin", 0);
	unlink(tempname);
    }
    return 0;
}
