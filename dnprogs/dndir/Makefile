# Makefile for dndir

include ../Makefile.common

PROG1=dndir

MANPAGES=dndir.1

PROG1OBJS=dndir.o

all: $(PROG1)

$(PROG1): $(PROG1OBJS) $(DEPLIBS)
	$(CXX) -o $@ $(PROG1OBJS) $(LIBS)

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
