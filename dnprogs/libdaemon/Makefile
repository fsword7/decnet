include ../Makefile.common

LIBOBJS=dnet_daemon.o dnetlog.o
PICOBJS=dnet_daemon.po dnetlog.po
MANPAGES3=dnet_daemon.3

LIBNAME=libdnet_daemon

SHAREDLIB=$(LIBNAME).so.$(VERSION)
STATICLIB=$(LIBNAME).a


all: $(STATICLIB) $(SHAREDLIB)

$(STATICLIB): $(LIBOBJS)
	ar -rv $@ $^

$(SHAREDLIB): $(PICOBJS)
	$(CC) $(CCFLAGS) -shared -o $@ -Wl,-soname=$(LIBNAME).so.$(MAJOR_VERSION) $(LIBCRYPT) $^
	ln -sf $(SHAREDLIB) $(LIBNAME).so.$(MAJOR_VERSION)
	ln -sf $(LIBNAME).so.$(MAJOR_VERSION) $(LIBNAME).so

.c.o:
	$(CC) $(CFLAGS) $(SYSCONF_PREFIX) -c -o $@ $<

.c.po:
	$(CC) $(CFLAGS) $(SYSCONF_PREFIX) -fPIC -c -o $@ $<

dep depend:	
	$(CC) $(CFLAGS) -MM *.c >.depend 2>/dev/null

clean:
	rm -f *.o *.po *.bak $(STATICLIB) $(SHAREDLIB) $(LIBNAME).so* .depend

install:
	install -m 0644 -s $(SHAREDLIB) $(libprefix)/lib
	install -m 0644 $(STATICLIB) $(libprefix)/lib
	ln -sf $(SHAREDLIB) $(libprefix)/lib/$(LIBNAME).so.$(MAJOR_VERSION)
	ln -sf $(LIBNAME).so.$(MAJOR_VERSION) $(libprefix)/lib/$(LIBNAME).so
	install -d $(prefix)/man/man3
	install -m 0644 $(MANPAGES3) $(prefix)/man/man3
	ln -sf dnet_daemon.3 $(prefix)/man/man3/dnet_accept.3
	ln -sf dnet_daemon.3 $(prefix)/man/man3/dnet_reject.3

.SUFFIXES: .po

ifeq (.depend,$(wildcard .depend))
include .depend
endif

