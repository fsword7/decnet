/******************************************************************************
    (c) 2002      P.J. Caulfield          patrick@debian.org

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


extern char *found_connerror(char *default_msg);
extern int found_getsockfd(void);
extern int found_write(char *buf, int len);
extern int found_read(void);
extern int found_setup_link(char *node, int object);
extern int process_cterm(char *buf, int len);

extern int debug;

