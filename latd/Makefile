# Makefile for LAT server

VERSION=0.9
PKGNAME=latd

#
# ARCH code taken from the Linux kernel sources.
#
ARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ -e s/arm.*/arm/ -e s/sa110/arm/)


PROG1=latd
PROG2=latcp
MANPAGES8=latd.8 latcp.8
MANPAGES5=latd.conf.5
PROG1OBJS=main.o utils.o server.o connection.o session.o \
	  serversession.o clientsession.o portsession.o services.o \
	  latcpcircuit.o lat_messages.o
PROG2OBJS=latcp.o utils.o

DEFS=-DVERSION=\"$(VERSION)\" -DLATCP_SOCKNAME=\"$(LATCPSOCK)\"
#DEFS+=#-DOLDSTUFF # -DVERBOSE_DEBUG -DNO_FORK
#DEFS+=-DVERBOSE_DEBUG -DNO_FORK

prefix=/usr/local

#  Look for libdnet and use it if it's there
ifeq (/usr/lib/libdnet.a,$(wildcard /usr/lib/libdnet.a))
DEFS+=-DHAVE_LIBDNET
LIBDNET=-ldnet
endif


OPTDEBUG=-g -O
CXX=g++
CXXFLAGS+=$(OPTDEBUG) $(DEFS) -pipe -Wstrict-prototypes -Wall -fno-rtti -fno-exceptions
LATCPSOCK="/var/run/latcp"

ifdef RELEASE
OPTDEBUG=-O2
endif

all: $(PROG1)  $(PROG2)

$(PROG1): depend $(PROG1OBJS) 
	$(CXX) -o $@ $(PROG1OBJS) $(LIBDNET) -lutil

$(PROG2): depend $(PROG2OBJS) 
	$(CXX) -o $@ $(PROG2OBJS) $(LIBDNET)

install:
	install -d $(prefix)/sbin
	install -d $(prefix)/man/man5
	install -d $(prefix)/man/man8
	install -m 0750 -s $(PROG1) $(prefix)/sbin
	install -m 0750 -s $(PROG2) $(prefix)/sbin
	install -m 0644 $(MANPAGES8) $(prefix)/man/man8
	install -m 0644 $(MANPAGES5) $(prefix)/man/man5

dep depend: .depend
	@makedepend *.cc *.c 2>/dev/null

.depend:
	@touch .depend

clean:
	makedepend
	rm -f $(PROG1) $(PROG2) *.o *.bak .depend

dist:
	makedepend
	rm  -f .depend
	cd ..; tar czf latd-$(VERSION).tar.gz latd/*.h latd/*.cc \
		latd/BUGS latd/README latd/NEWS latd/Makefile \
                latd/WARRANTY latd/COPYING latd/INSTALL latd/TODO \
		latd/*.[1-8] latd.conf

rpm:
	echo "%_topdir `pwd`" > ~/.rpmmacros
	echo "macrofiles $(HOME)/.rpmmacros"
	rm -rf rpmbuild
	make clean
	make prefix=/usr RELEASE=true
	make prefix=`pwd`/rpmbuild/usr install
	make dist
	install -d rpmbuild/usr/doc
	install -d rpmbuild/etc
	install latd.conf rpmbuild/etc/latd.conf
	sed -e's/%%PACKAGENAME%%/$(PKGNAME)/g'                          \
	    -e's/%%VERSION%%/$(VERSION)/g'                              \
	    -e's/%%MAJOR_VERSION%%/$(MAJOR_VERSION)/g'                  \
	    -e's@%%PREFIX%%@/usr@g'                                     \
	   < rpm.spec >$(PKGNAME).spec
	rm -rf BUILD
	mkdir BUILD
	install README NEWS BUILD
	mkdir -p RPMS/$(ARCH)
	rpm -bb --buildroot `pwd`/rpmbuild -v $(PKGNAME).spec
	rm -f $(PKGNAME).spec ~/.rpmrc ~/.rpmmacros
	rm -rf BUILD



# DO NOT DELETE THIS LINE -- make  depend  depends  on it.
