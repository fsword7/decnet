//dnetstat.c:

/*
    copyright 2008 Philipp 'ph3-der-loewe' Schafft <lion@lion.leolix.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <netdnet/dn.h>
#include <netdnet/dnetdb.h>

#define DNNS_FILE "/proc/net/decnet"

struct dn_nse {
 char node[8];
 char object[32];
};

char * progname = NULL;
int numeric = 0;

void usage (void) {
 fprintf(stderr, "Usage: %s [OPTIONS]\n", progname);
 fprintf(stderr, "\n");
 fprintf(stderr, "Options:\n");
 fprintf(stderr, " -h   --help        this help\n"
                 " -n                 numerical mode (do not show node names but addresses)\n"
        );
}

int prep_addr (char * buf, char * object) {
 struct nodeent * ne;

 *index(buf, '/') = 0;

  if ( strcmp(buf, "0.0") == 0 && ! numeric ) {
   strcpy(buf, "*");
  } else if ( !numeric ) {
   if ( (ne = getnodebyname(buf)) != NULL ) {
    if ( (ne = getnodebyaddr((const char *)ne->n_addr, ne->n_length, ne->n_addrtype)) != NULL ) {
     strcpy(buf, ne->n_name);
    }
   }
  }

 strcat(buf, "::");

 if ( strcmp(object, "0") == 0 && ! numeric ) {
  strcat(buf, "*");
 } else {
  strcat(buf, object);
 }
 return 0;
}

char * state_ktou (char * state) {
 if ( strcmp(state, "OPEN") == 0 ) {
  return "LISTEN";
 } else if ( strcmp(state, "RUN") == 0 ) {
  return "ESTABLISHED";
 } else if ( strcmp(state, "RJ") == 0 ) {
  return "REJECTED";
 } else if ( strcmp(state, "DN") == 0 ) {
  return "DISCNOTIFY";
 } else if ( strcmp(state, "DIC") == 0 ) {
  return "DISCONNECTED";
 }

 return state;
}

int proc_file (FILE * fh) {
 struct dn_nse local, remote;
 char state[32], immed[32];
 char buf[1024];
 char * lbuf = buf, * rbuf = buf + 512;
 char * dir = ""; // max be "UNI" or "BI", "IN", "OUT"
 int unused;

 if ( fgets(buf, 1024, fh) == NULL ) {
  fprintf(stderr, "Error: can not read banner from file\n");
  return -1;
 }

 if ( strcmp(buf, "Local                                              Remote\n") != 0 ) {
  fprintf(stderr, "Error: invalid file format\n");
  return -1;
 }

 while (fscanf(fh, "%s %04d:%04d %04d:%04d %01d %16s"
                   "%s %04d:%04d %04d:%04d %01d %16s"
                   "%4s %s\n",
                   lbuf, &unused, &unused, &unused, &unused, &unused, local.object,
                   rbuf, &unused, &unused, &unused, &unused, &unused, remote.object,
                   state, immed
                   ) == 16) {
  prep_addr(lbuf, local.object);
  prep_addr(rbuf, remote.object);

  if ( strcmp(state, "OPEN") != 0 ) {
   immed[0] = 0;
   dir = "";
  } else {
   dir = "IN";
  }

  printf("decnet %-24s %-24s %-3s %-12s %s\n", lbuf, rbuf, dir, state_ktou(state), immed);
 }

 return 0;
}

int main (int argc, char * argv[]) {
 FILE * fh = NULL;
 int i;
 char * k;
 char * file = DNNS_FILE;

 progname = argv[0];

 for (i = 1; i < argc; i++) {
  k = argv[i];
  if ( strcmp(k, "-n") == 0 ) {
   numeric = 1;
  } else if ( strcmp(k, "-h") == 0 || strcmp(k, "--help") == 0 ) {
   usage();
   return 0;
  } else {
   fprintf(stderr, "Error: unknown parameter %s\n", k);
   usage();
   return 1;
  }
 }

 if ( (fh = fopen(file, "r")) == NULL ) {
  fprintf(stderr, "Error: can not open DECnet netstat file: %s: %s\n", file, strerror(errno));
  return 2;
 }

//      |Proto Recv-Q Send-Q Local Address           Foreign Address         State       PID/Program name
//      |decnet *::29                    *::0                         LISTEN       IMMED
 printf("Active DECnet sockets (servers and established)\n");
 printf("Proto  Local Address            Foreign Address          Dir State        Accept mode\n");
 proc_file(fh);

 fclose(fh);

 return 0;
}

//ll
