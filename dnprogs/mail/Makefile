# Makefile for mail progs

include ../Makefile.common

PROG1=vmsmaild
PROG2=sendvmsmail

UULIB=uulib/libuu.a

MANPAGES5=vmsmail.conf.5
MANPAGES8=vmsmaild.8 sendvmsmail.8

PROG1OBJS=vmsmaild.o receive.o configfile.o
PROG2OBJS=sendvmsmail.o configfile.o

CDEFS+=-Iuulib -DPROTOTYPES

all: $(UULIB) $(PROG1) $(PROG2)

$(UULIB): 
	make -C uulib

.c.o:
	$(CC) $(CFLAGS) $(SYSCONF_PREFIX) -c -o $@ $<

$(PROG1): $(PROG1OBJS) $(DEPLIBDNET) $(DEPLIBDAEMON) $(UULIB)
	$(CC) $(CFLAGS) -o $@ $(PROG1OBJS) $(LIBDNET) $(UULIB) $(LIBDAEMON)

$(PROG2): $(PROG2OBJS) $(DEPLIBDNET) $(UULIB)
	$(CC) $(CFLAGS) -o $@ $(PROG2OBJS) $(LIBDNET) $(UULIB)

install:
	install -d $(prefix)/bin
	install -d $(prefix)/man/man5
	install -d $(prefix)/man/man8
	install -m 0755 -s $(PROG1) $(prefix)/sbin
	install -m 0755 -s $(PROG2) $(prefix)/sbin
	install -m 0644 $(MANPAGES5) $(prefix)/man/man5
	install -m 0644 $(MANPAGES8) $(prefix)/man/man8

dep depend:	
	$(CC) $(CFLAGS) -MM *.c >.depend 2>/dev/null

clean:
	rm -f $(PROG1) $(PROG2) *.o *.bak .depend
	make -Cuulib clean


ifeq (.depend,$(wildcard .depend))
include .depend
endif
