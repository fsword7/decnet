# Makefile for dapfs

include ../Makefile.common

PROG1=dapfs

MANPAGES=dapfs.1

PROG1OBJS=dapfs.o dapfs_dap.o filenames.o

all: $(PROG1)

CFLAGS=-g -I../include -Wall
LDFLAGS=-g

$(PROG1): $(PROG1OBJS) $(DEPLIBS)
	g++ -o$(PROG1) $(LDFLAGS) $(PROG1OBJS) -L../librms -lrms  -lfuse

install:
	install -d $(prefix)/bin
	install -d $(manprefix)/man/man1
	install -m 0755 $(STRIPBIN) $(PROG1) $(prefix)/bin
	install -m 0644 $(MANPAGES) $(manprefix)/man/man1

dep depend:	
	$(CXX) $(CXXFLAGS) -MM *.cc >.depend 2>/dev/null

clean:
	rm -f $(PROG1) *.o *.bak .depend


ifeq (.depend,$(wildcard .depend))
include .depend
endif

