# Makefile for DECnet super server

include ../Makefile.common

PROG1=dnetd

MANPAGES8=dnetd.8
MANPAGES5=dnetd.conf.5

PROG1OBJS=dnetd.o task_server.o

all: $(PROG1)

.c.o:
	$(CC) $(CFLAGS) $(BINARY_PREFIX) -c -o $@ $<

$(PROG1): $(PROG1OBJS) $(DEPLIBDNET) $(DEPLIBDAEMON)
	$(CC) $(CFLAGS) -o $@ $(PROG1OBJS) $(LIBDAEMON) $(LIBDNET) $(PTSLIBS)

install:
	install -d $(prefix)/bin
	install -d $(prefix)/man/man5
	install -d $(prefix)/man/man8
	install -m 0750 -s $(PROG1) $(prefix)/sbin
	install -m 0644 $(MANPAGES5) $(prefix)/man/man5
	install -m 0644 $(MANPAGES8) $(prefix)/man/man8
	if [ ! -f $(sysconfprefix)/etc/dnetd.conf ]; then install -m 640 dnetd.conf $(sysconfprefix)/etc; fi

dep depend:	
	makedepend *.cc *.c 2>/dev/null

clean:
	makedepend
	rm -f $(PROG1) *.o *.bak


# DO NOT DELETE THIS LINE -- make  depend  depends  on it.
