# Makefile for LAT server

VERSION=1.0
PKGNAME=latd


PROG1=latd
PROG2=latcp
MANPAGES8=latd.8 latcp.8
MANPAGES5=latd.conf.5
PROG1OBJS=main.o utils.o server.o connection.o session.o \
	  serversession.o clientsession.o portsession.o services.o \
	  latcpcircuit.o lat_messages.o
PROG2OBJS=latcp.o utils.o

DEFS=-DVERSION=\"$(VERSION)\" -DLATCP_SOCKNAME=\"$(LATCPSOCK)\"
DEFS+=-DUSE_OPENPTY #-DSETLOGIN_HOST
#DEFS+=-DVERBOSE_DEBUG -DNO_FORK

prefix=/usr/local
LATCPSOCK="/var/run/latcp"
OPTDEBUG=-g -O
CXX=g++
CXXFLAGS+=$(OPTDEBUG) $(DEFS) -pipe -Wstrict-prototypes -Wall -fno-rtti -fno-exceptions

#
# For DEB builds
#
ifdef DESTDIR
prefix=$(DESTDIR)/usr
endif


# Look for libdnet and use it if it's there. Unless we are building a
# release package in which case don't use it at all.
ifndef RELEASE
ifeq (/usr/lib/libdnet.a,$(wildcard /usr/lib/libdnet.a))
DEFS+=-DHAVE_LIBDNET
LIBDNET=-ldnet
endif
endif


# For a released binary, make it portable and optimised
ifdef RELEASE
OPTDEBUG=-O2
endif

#
# Targets:
#
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
	rm -rf debian/tmp

dist:
	makedepend
	rm  -f .depend
	rm -rf debian/tmp
	cd ..; tar czf latd-$(VERSION).tar.gz latd/*.h latd/*.cc \
		latd/BUGS latd/README latd/NEWS latd/Makefile \
                latd/WARRANTY latd/COPYING latd/INSTALL latd/TODO \
		latd/*.[1-8] latd/latd.conf latd/startlat.sh \
		latd/rpm.spec latd/debian-changelog latd/debian/*

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
	@if [ $$UID != 0 ]; then echo You must be root to make DEBs; exit 1; fi
	make clean
	make prefix=/usr RELEASE=true
	sed -e's/%%PACKAGENAME%%/$(PKGNAME)/g'        \
	    -e's/%%VERSION%%/$(VERSION)/g'            \
	    -e's@%%PREFIX%%@$(prefix)@g'              \
	    -e"s/%%DATE%%/`date --rfc-822`/g"         \
	   <debian-changelog >debian/changelog
	echo >>debian/changelog
	cat NEWS >>debian/changelog
	if [ -L ../$(PKGNAME)-$(VERSION) ]; then rm ../$(PKGNAME)-$(VERSION); fi
	if [ ! -d ../$(PKGNAME)-$(VERSION) ]; then cd ..; ln -s $(PKGNAME) $(PKGNAME)-$(VERSION); fi
	cd ../$(PKGNAME)-$(VERSION); debian/rules binary-arch
	if [ -L ../$(PKGNAME)-$(VERSION) ]; then rm ../$(PKGNAME)-$(VERSION); fi


# DO NOT DELETE THIS LINE -- make  depend  depends  on it.
