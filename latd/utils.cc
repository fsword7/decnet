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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>
#include <stdio.h>

#include "utils.h"

void add_string(unsigned char *packet, int *ptr, 
		const unsigned char *string)
{
    int len = strlen((char *)string);
    
    packet[(*ptr)++] = len;
    strcpy((char *)packet + *ptr, (char *)string);
    *ptr += len;
}

void get_string(unsigned char *packet, int *ptr, 
		unsigned char *string)
{
    int len = packet[(*ptr)++];
    
    strncpy((char*)string, (char*)packet + *ptr, len);
    string[len] = '\0';
    *ptr += len;
}

#ifdef VERBOSE_DEBUG
void pjc_debuglog(char *fmt, ...)
{
  va_list ap;
  
  va_start(ap, fmt);

  vfprintf(stderr, fmt, ap);
}
#endif


#ifdef INTERNAL_OPENPTY
int pjc_openpty(int *master, int *slave, char *a, char *b, char *d)
{    
    char ptyname[] = "/dev/ptyCP";
    char c;
    char *line=NULL;
    int i;
    int pty=-1, t;
    int gotpty=0;
    struct stat stb;

    for (c='p'; c <= 'z'; c++)
    {
	line = ptyname;
	line[strlen("/dev/pty")] = c;
	line[strlen("/dev/ptyC")] = '0';
	if (stat(line,&stb) < 0)
	    break; 
	for (i=0; i < 16; i++)
	{
	    line[strlen("/dev/ptyC")]= "0123456789abcdef"[i];
	    if ( (pty=open(line,O_RDWR)) > 0)	
	    {
		gotpty = 1;
		break;
	    }
	}
	if (gotpty) break;
    }

    if (!gotpty) 
    {
	debuglog(("No ptys available for connection"));
	return -1;
    }
    

    line[strlen("/dev/")] = 't';
    if ( (t=open(line,O_RDWR)) < 0) 
    {
	debuglog(("Error connecting to physical terminal: %m"));
	return -1;
    }
    *master = pty;
    *slave = t;
    return 0;
}
#endif
