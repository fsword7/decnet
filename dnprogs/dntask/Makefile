# Makefile for dntask

include ../Makefile.common

PROG1=dntask

MANPAGES=dntask.1

PROG1OBJS=dntask.o

all: $(PROG1)

$(PROG1): $(PROG1OBJS) $(DEPLIBDNET)
	$(CC) -o $@ $(PROG1OBJS) $(LIBDNET)

install:
	install -d $(prefix)/bin
	install -d $(prefix)/man/man1
	install -m 0755 -s $(PROG1) $(prefix)/bin
	install -m 0644 $(MANPAGES) $(prefix)/man/man1

dep depend:	
	makedepend *.cc *.c 2>/dev/null

clean:
	makedepend
	rm -f $(PROG1) *.o *.bak


# DO NOT DELETE THIS LINE -- make  depend  depends  on it.
