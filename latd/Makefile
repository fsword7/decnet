# Makefile for LAT server

VERSION=1.03
PKGNAME=latd


PROG1=latd
PROG2=latcp
PROG3=llogin
MANPAGES1=llogin.1
MANPAGES5=latd.conf.5
MANPAGES8=latd.8 latcp.8
PROG1OBJS=main.o utils.o server.o connection.o session.o \
	  serversession.o clientsession.o portsession.o services.o \
	  latcpcircuit.o lat_messages.o lloginsession.o llogincircuit.o circuit.o
PROG2OBJS=latcp.o utils.o
PROG3OBJS=llogin.o utils.o

DEFS=-DVERSION=\"$(VERSION)\" -DLATCP_SOCKNAME=\"$(LATCPSOCK)\" -DLLOGIN_SOCKNAME=\"$(LLOGINSOCK)\"
DEFS+=-DUSE_OPENPTY #-DSETLOGIN_HOST
#DEFS+=-DVERBOSE_DEBUG -DNO_FORK -DDEBUG_MALLOC

prefix=/usr/local
LATCPSOCK="/var/run/latcp"
LLOGINSOCK="/var/run/latlogin"
OPTDEBUG=-g
CXX=g++ 
CXXFLAGS+=$(OPTDEBUG) $(DEFS) -pipe -Wstrict-prototypes -Wall -fno-rtti -fno-exceptions

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
all: $(PROG1) $(PROG2) $(PROG3)

$(PROG1): depend $(PROG1OBJS) 
	$(CXX) -o $@ $(PROG1OBJS) -lutil

$(PROG2): depend $(PROG2OBJS) 
	$(CXX) -o $@ $(PROG2OBJS)

$(PROG3): depend $(PROG3OBJS) 
	$(CXX) -o $@ $(PROG3OBJS)

install:
	install -d $(prefix)/sbin
	install -d $(prefix)/bin
	install -d $(prefix)/man/man1
	install -d $(prefix)/man/man5
	install -d $(prefix)/man/man8
	install -m 0750 -s $(PROG1) $(prefix)/sbin
	install -m 0750 -s $(PROG2) $(prefix)/sbin
	install -m 0755 -s $(PROG3) $(prefix)/bin
	install -m 0644 $(MANPAGES1) $(prefix)/man/man1
	install -m 0644 $(MANPAGES5) $(prefix)/man/man5
	install -m 0644 $(MANPAGES8) $(prefix)/man/man8

dep depend: .depend
	@makedepend *.cc *.c 2>/dev/null

.depend:
	@touch .depend

clean:
	makedepend
	rm -f $(PROG1) $(PROG2) $(PROG3) *.o *.bak .depend 
	rm -rf debian/tmp

dist:
	makedepend
	rm  -f .depend
	rm -rf debian/tmp
	cd ..; tar czf latd-$(VERSION).tar.gz latd/*.h latd/*.cc \
		latd/BUGS latd/README latd/NEWS latd/Makefile \
                latd/WARRANTY latd/COPYING latd/INSTALL latd/TODO \
		latd/*.[1-8] latd/latd.conf latd/startlat.sh \
		latd/rpm.spec latd/debian/*

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
	rpm -bb --buildroot `pwd`/rpmbuild --rcfile .rpmrc -v $(PKGNAME).spec
	rm -f $(PKGNAME).spec .rpmrc .rpmmacros

#
# Make Debian package.
#
deb:
	dpkg-buildpackage -rfakeroot


# DO NOT DELETE THIS LINE -- make  depend  depends  on it.
