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


void add_string(unsigned char *packet, int *ptr, 
		unsigned char *string);

void get_string(unsigned char *packet, int *ptr, 
		unsigned char *string);

#ifdef OLDSTUFF
#define INTERNAL_OPENPTY
#else
#include <pty.h>
#endif

// RedHat 5 has a broken openpty()
#ifdef INTERNAL_OPENPTY
int pjc_openpty(int*,int*, char*,char*,char*);
#define openpty pjc_openpty
#endif

#ifdef VERBOSE_DEBUG
#define debuglog(x) pjc_debuglog x

#include <stdarg.h>
extern void pjc_debuglog(char *,...);
#else
#define debuglog(x)
#endif
