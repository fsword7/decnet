# Makefile for dnsubmit

include ../Makefile.common

PROG1=dnsubmit
PROG2=dnprint

MANPAGES=dnsubmit.1

PROG1OBJS=dnsubmit.o

all: $(PROG1) $(PROG2)

$(PROG1): $(PROG1OBJS)
	$(CXX) -o $@ $(PROG1OBJS) $(LIBS)

$(PROG2): $(PROG1)
	ln -sf $< $@

install:
	install -d $(prefix)/bin
	install -d $(prefix)/man/man1
	install -m 0755 -s $(PROG1) $(prefix)/bin
	ln -sf $(PROG1) $(prefix)/bin/$(PROG2)
	install -m 0644 $(MANPAGES) $(prefix)/man/man1
	ln -sf $(PROG1).1 $(prefix)/man/man1/$(PROG2).1


dep depend:	
	makedepend *.cc *.c 2>/dev/null

clean:
	makedepend
	rm -f $(PROG2) $(PROG1) *.o *.bak


# DO NOT DELETE THIS LINE -- make  depend  depends  on it.
