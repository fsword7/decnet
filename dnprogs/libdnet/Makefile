#--------------------------------------------------------------------------
# Makefile for libdnet
#--------------------------------------------------------------------------
include ../Makefile.common
LD=gcc
LDFLAGS=-O -shared
AR=ar
ARFLAGS=rcs
MANPAGES3=dnet_htoa.3 dnet_ntoa.3 dnet_addr.3 dnet_conn.3 \
	getnodeadd.3 getnodebyname.3 setnodeent.3 libdnet.3 \
	getnodebyaddr.3 dnet_getnode.3 dnet_eof.3
LIBOBJS :=dnet_htoa.o dnet_ntoa.o dnet_addr.o dnet_conn.o getnodeadd.o \
	getnodebyname.o getnodebyaddr.o setnodeent.o getexecdev.o \
	getnodename.o setnodename.o dnet_getnode.o dnet_pton.o dnet_ntop.o \
	dnet_recv.o dnet_eof.o
PICOBJS:=dnet_htoa.po dnet_ntoa.po dnet_addr.po dnet_conn.po getnodeadd.po \
	getnodebyname.po getnodebyaddr.po setnodeent.po getexecdev.po \
	getnodename.po setnodename.po dnet_getnode.po dnet_pton.po dnet_ntop.po\
	dnet_recv.po dnet_eof.po

LIBNAME=libdnet
SHAREDLIB=$(LIBNAME).so.$(VERSION)

all:  libdnet.a $(SHAREDLIB)

dep:

libdnet.a: ${LIBOBJS} 
	${AR} ${ARFLAGS} libdnet.a ${LIBOBJS}
  
$(SHAREDLIB): ${PICOBJS}
	${LD} ${LDFLAGS} -o $@ ${PICOBJS} -soname=libdnet.so.$(MAJOR_VERSION) -lc
	ln -sf $(SHAREDLIB) $(LIBNAME).so.$(MAJOR_VERSION)
	ln -sf $(LIBNAME).so.$(MAJOR_VERSION) $(LIBNAME).so

.c.o:
	$(CC) $(CFLAGS) $(SYSCONF_PREFIX) -c -o $@ $<

.c.po:
	$(CC) $(CFLAGS) -fPIC -shared $(SYSCONF_PREFIX) -c -o $@ $<

install: libdnet.a $(SHAREDLIB)
	install -d $(libprefix)/lib
	install -m 0644 libdnet.a $(libprefix)/lib/libdnet.a
	install -m 0644 -s $(SHAREDLIB) $(libprefix)/lib/$(SHAREDLIB)
	ln -sf $(SHAREDLIB) $(libprefix)/lib/$(LIBNAME).so.$(MAJOR_VERSION)
	ln -sf $(SHAREDLIB) $(libprefix)/lib/$(LIBNAME).so
	ln -sf $(LIBNAME).so.$(MAJOR_VERSION) $(libprefix)/lib/$(LIBNAME).so

	install -d $(prefix)/man/man3
	install -m 0644 $(MANPAGES3) $(prefix)/man/man3
	ln -sf dnet_getnode.3 $(prefix)/man/man3/dnet_nextnode.3
	ln -sf dnet_getnode.3 $(prefix)/man/man3/dnet_endnode.3

clean:
	rm -f *.a *.o *.po *.so* *~

.SUFFIXES: .po

# DO NOT DELETE
