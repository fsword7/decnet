# Makefile for dndel

include ../Makefile.common

PROG1=dndel

MANPAGES=dndel.1

PROG1OBJS=dndel.o

all: $(PROG1)

$(PROG1): $(PROG1OBJS) $(DEPLIBS)
	$(CXX) -o $@ $(PROG1OBJS) $(LIBS)

install:
	install -d $(prefix)/bin
	install -d $(prefix)/man/man1
	install -m 0755 -s $(PROG1) $(prefix)/bin
	install -m 0644 $(MANPAGES) $(prefix)/man/man1

dep depend:	
	$(CXX) $(CXXFLAGS) -MM *.cc >.depend 2>/dev/null

clean:
	rm -f $(PROG1) *.o *.bak .depend


ifeq (.depend,$(wildcard .depend))
include .depend
endif
