# Makefile for LAT server

VERSION=1.15
PKGNAME=latd


PROG1=latd
PROG2=latcp
PROG3=llogin
PROG4=moprc
MANPAGES1=llogin.1
MANPAGES5=latd.conf.5
MANPAGES8=latd.8 latcp.8 moprc.8
PROG1OBJS=main.o utils.o server.o connection.o session.o \
	  serversession.o clientsession.o queuedsession.o services.o \
	  latcpcircuit.o lat_messages.o lloginsession.o llogincircuit.o \
	  circuit.o localport.o localportsession.o interfaces.o interfaces-linux.o
PROG2OBJS=latcp.o utils.o
PROG3OBJS=llogin.o utils.o
PROG4OBJS=moprc.o

DEFS=-DVERSION=\"$(VERSION)\" -DLATCP_SOCKNAME=\"$(LATCPSOCK)\" -DLLOGIN_SOCKNAME=\"$(LLOGINSOCK)\"
DEFS+=-DUSE_OPENPTY #-DSETLOGIN_HOST

# These are debugging options, know what you are doing before setting these.
#DEFS+=-DVERBOSE_DEBUG -DNO_FORK -DDEBUG_MALLOC
#DEFS+=-DREALLY_VERBOSE_DEBUGLOG
#DEFS+=-DPACKET_LOSS=8

prefix=/usr/local
LATCPSOCK="/var/run/latcp"
LLOGINSOCK="/var/run/latlogin"
OPTDEBUG=-g
CXX=g++
CC=gcc
CXXFLAGS+=$(OPTDEBUG) $(DEFS) -pipe -Wstrict-prototypes -Wall -fno-rtti -fno-exceptions
CFLAGS+=$(OPTDEBUG) $(DEFS) -pipe -Wstrict-prototypes -Wall

# ARCH code taken from the Linux kernel sources.
#
ARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ -e s/arm.*/arm/ -e s/sa110/arm/)

#
# For DEB builds
#
ifdef DESTDIR
prefix=$(DESTDIR)/usr
endif

# For a released binary, make it optimised
ifdef RELEASE
OPTDEBUG=-O2
endif

#
# Targets:
#
all: $(PROG1) $(PROG2) $(PROG3) $(PROG4)

$(PROG1): depend $(PROG1OBJS)
	$(CXX) -o $@ $(PROG1OBJS) -lutil

$(PROG2): depend $(PROG2OBJS)
	$(CXX) -o $@ $(PROG2OBJS)

$(PROG3): depend $(PROG3OBJS)
	$(CXX) -o $@ $(PROG3OBJS)

$(PROG4): depend $(PROG4OBJS)
	$(CC) $(CFLAGS) -o $@ $(PROG4OBJS)

install:
	install -d $(prefix)/sbin
	install -d $(prefix)/bin
	install -d $(prefix)/man/man1
	install -d $(prefix)/man/man5
	install -d $(prefix)/man/man8
	install -m 0750 -s $(PROG1) $(prefix)/sbin
	install -m 0750 -s $(PROG2) $(prefix)/sbin
	install -m 0755 -s $(PROG3) $(prefix)/bin
	install -m 0750 -s $(PROG4) $(prefix)/sbin
	install -m 0644 $(MANPAGES1) $(prefix)/man/man1
	install -m 0644 $(MANPAGES5) $(prefix)/man/man5
	install -m 0644 $(MANPAGES8) $(prefix)/man/man8

dep depend:
	@$(CXX) -MM *.cc >.depend 2>/dev/null

clean:
	rm -f .depend
	rm -f $(PROG1) $(PROG2) $(PROG3) *.o *.bak .depend core
	rm -rf debian/tmp

debclean:
	debian/rules clean
dist:
	rm  -f .depend
	rm -rf debian/tmp
	cd ..; tar czf latd-$(VERSION).tar.gz latd/*.h latd/*.cc latd/*.c \
		latd/BUGS latd/README latd/NEWS latd/Makefile latd/rpm.spec \
                latd/WARRANTY latd/COPYING latd/INSTALL latd/TODO \
		latd/*.[1-8] latd/latd.conf latd/startlat.sh \
		latd/rpm.spec latd/*.html latd/debian/*

rpm:
	echo "%_topdir `pwd`" > .rpmmacros
	echo "`rpm --showrc|grep \^macrofiles`:`pwd`/.rpmmacros" >.rpmrc
	rm -rf rpmbuild BUILD RPMS
	make clean
	make prefix=/usr RELEASE=true
	make prefix=`pwd`/rpmbuild/usr install
	make dist
	install -d rpmbuild/usr/doc
	install -d rpmbuild/etc
	install -d rpmbuild/usr/doc/$(PKGNAME)-$(VERSION)
	install latd.conf rpmbuild/etc/latd.conf
	install -Dm 0700 startlat.sh rpmbuild/etc/rc.d/init.d/lat
	install -d rpmbuild/etc/rc.d/rc3.d
	ln -sf ../init.d/lat rpmbuild/etc/rc.d/rc3.d/S79lat
	ln -sf ../init.d/lat rpmbuild/etc/rc.d/rc3.d/K79lat
	sed -e's/%%PACKAGENAME%%/$(PKGNAME)/g'                          \
	    -e's/%%VERSION%%/$(VERSION)/g'                              \
	    -e's/%%MAJOR_VERSION%%/$(MAJOR_VERSION)/g'                  \
	    -e's@%%PREFIX%%@/usr@g'                                     \
	   < rpm.spec >$(PKGNAME).spec
	mkdir BUILD
	install README NEWS BUILD
	mkdir -p RPMS
	rpm -bb --target=$(ARCH) --buildroot `pwd`/rpmbuild --rcfile .rpmrc -v $(PKGNAME).spec
	rm -f $(PKGNAME).spec .rpmrc .rpmmacros

#
# Make Debian package.
#
deb:
	dpkg-buildpackage -rfakeroot


ifeq (.depend,$(wildcard .depend))
include .depend
endif
