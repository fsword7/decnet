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


/* Foundation services routines */
extern char *found_connerror(char *default_msg);
extern int found_getsockfd(void);
extern int found_write(char *buf, int len);
extern int found_read(void);
extern int found_setup_link(char *node, int object, int (*processor)(char *, int));
extern int found_common_write(char *buf, int len);

/* cterm/dterm routines */
extern int cterm_send_input(char *buf, int len, int flags);
extern int cterm_process_network(char *buf, int len);

/* TTY routines */
extern int tty_write(char *buf, int len);
extern void tty_set_terminators(char *buf, int len);
extern void tty_start_read(char *prompt, int len, int promptlen);
extern void tty_set_timeout(unsigned short to);
extern void tty_set_maxlen(unsigned short len);


/* Global variables */
extern int debug;

