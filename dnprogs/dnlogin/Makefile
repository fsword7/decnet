# Makefile for dnlogin

include ../Makefile.common

CFLAGS+=-I../apps

PROG1=dnlogin

MANPAGES=dnlogin.1

PROG1OBJS=dnlogin.o found.o cterm.o tty.o

all: $(PROG1)

$(PROG1): $(PROG1OBJS) $(DEPLIBDNET)
	$(CC) -o $@ $(PROG1OBJS) $(LIBDNET)

install:
	install -d $(prefix)/bin
	install -d $(prefix)/man/man1
	install -m 0755 -s $(PROG1) $(prefix)/bin
	install -m 0644 $(MANPAGES) $(prefix)/man/man1

dep depend:
	$(CC) $(CFLAGS) -MM *.c >.depend 2>/dev/null

clean:
	rm -f $(PROG1) *.o *.bak .depend


ifeq (.depend,$(wildcard .depend))
include .depend
endif

