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
extern char *found_connerror(void);
extern int found_getsockfd(void);
extern int found_write(char *buf, int len);
extern int found_read(void);
extern int found_setup_link(char *node, int object, int (*processor)(char *, int));
extern int found_common_write(char *buf, int len);

/* cterm/dterm routines */
extern int cterm_send_input(char *buf, int len, int term_pos, int flags);
extern int cterm_send_oob(char, int);
extern int cterm_process_network(char *buf, int len);

/* TTY routines */
extern int  tty_write(char *buf, int len);
extern void tty_set_terminators(char *buf, int len);
extern void tty_timeout(void);
extern void tty_set_default_terminators(void);
extern void tty_start_read(char *prompt, int len, int promptlen);
extern void tty_set_timeout(unsigned short to);
extern void tty_set_maxlen(unsigned short len);
extern void tty_send_unread(void);
extern int  tty_process_terminal(char *inbuf, int len);
extern int  tty_setup(char *name, int setup);
extern void tty_clear_typeahead(void);
extern void tty_set_noecho(void);
extern void tty_echo_terminator(int a);
extern void tty_set_discard(int onoff);
extern int  tty_set_escape_proc(int onoff);
extern void tty_set_uppercase(int onoff);
extern void tty_allow_edit(int onoff);

extern int (*send_input)(char *buf, int len, int term_pos, int flags);
extern int (*send_oob)(char, int);

/* Global variables */
extern int debug;

