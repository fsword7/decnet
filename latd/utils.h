/******************************************************************************
    (c) 2002 Patrick Caulfield                 patrick@debian.org

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
		const unsigned char *string);

void get_string(unsigned char *packet, int *ptr, 
		unsigned char *string);


int expand_issue(const char *original, int len,
		 char *newstring, int maxlen,
		 const char *servicename);

#ifndef HAVE_OPENPTY
#define INTERNAL_OPENPTY
#else
#ifdef HAVE_PTY_H
#include <pty.h>
#endif /* HAVE_PTY_H */
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif /* HAVE_TERMIOS_H */
#ifdef HAVE_LIBUTIL_H
#include <libutil.h>
#endif /* HAVE_LIBUTIL_H */
#ifdef HAVE_UTIL_H
#include <util.h>
#endif /* HAVE_UTIL_H */
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
