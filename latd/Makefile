# Makefile for LAT server

VERSION=1.02
PKGNAME=latd


PROG1=latd
PROG2=latcp
PROG3=llogin
MANPAGES8=latd.8 latcp.8
MANPAGES5=latd.conf.5
PROG1OBJS=main.o utils.o server.o connection.o session.o \
	  serversession.o clientsession.o portsession.o services.o \
	  latcpcircuit.o lat_messages.o lloginsession.o llogincircuit.o circuit.o
PROG2OBJS=latcp.o utils.o
PROG3OBJS=llogin.o utils.o

DEFS=-DVERSION=\"$(VERSION)\" -DLATCP_SOCKNAME=\"$(LATCPSOCK)\" -DLLOGIN_SOCKNAME=\"$(LLOGINSOCK)\"
DEFS+=-DUSE_OPENPTY #-DSETLOGIN_HOST
#DEFS+=-DVERBOSE_DEBUG -DNO_FORK

prefix=/usr/local
LATCPSOCK="/var/run/latcp"
LLOGINSOCK="/var/run/latlogin"
OPTDEBUG=-g -O
CXX=g++
CXXFLAGS+=$(OPTDEBUG) $(DEFS) -pipe -Wstrict-prototypes -Wall -fno-rtti -fno-exceptions

#
# For DEB builds
#
ifdef DESTDIR
prefix=$(DESTDIR)/usr
endif

# For a released binary, make it portable and optimised
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
	install -d $(prefix)/man/man5
	install -d $(prefix)/man/man8
	install -m 0750 -s $(PROG1) $(prefix)/sbin
	install -m 0750 -s $(PROG2) $(prefix)/sbin
	install -m 0750 -s $(PROG3) $(prefix)/bin
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

circuit.o: /usr/include/sys/types.h /usr/include/features.h
circuit.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
circuit.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
circuit.o: /usr/include/bits/sched.h /usr/include/time.h
circuit.o: /usr/include/endian.h /usr/include/bits/endian.h
circuit.o: /usr/include/sys/select.h /usr/include/bits/select.h
circuit.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
circuit.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
circuit.o: /usr/include/bits/confname.h /usr/include/getopt.h lat.h latcp.h
circuit.o: utils.h services.h session.h connection.h circuit.h
circuit.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
circuit.o: /usr/include/wchar.h /usr/include/bits/wchar.h
circuit.o: /usr/include/gconv.h /usr/include/bits/stdio_lim.h latcpcircuit.h
circuit.o: server.h
clientsession.o: /usr/include/sys/types.h /usr/include/features.h
clientsession.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
clientsession.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
clientsession.o: /usr/include/bits/sched.h /usr/include/time.h
clientsession.o: /usr/include/endian.h /usr/include/bits/endian.h
clientsession.o: /usr/include/sys/select.h /usr/include/bits/select.h
clientsession.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
clientsession.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
clientsession.o: /usr/include/stdio.h /usr/include/libio.h
clientsession.o: /usr/include/_G_config.h /usr/include/wchar.h
clientsession.o: /usr/include/bits/wchar.h /usr/include/gconv.h
clientsession.o: /usr/include/bits/stdio_lim.h /usr/include/fcntl.h
clientsession.o: /usr/include/bits/fcntl.h /usr/include/unistd.h
clientsession.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
clientsession.o: /usr/include/getopt.h /usr/include/termios.h
clientsession.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
clientsession.o: lat.h utils.h session.h connection.h circuit.h
clientsession.o: latcpcircuit.h server.h clientsession.h lat_messages.h
connection.o: /usr/include/stdio.h /usr/include/features.h
connection.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
connection.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
connection.o: /usr/include/bits/sched.h /usr/include/libio.h
connection.o: /usr/include/_G_config.h /usr/include/wchar.h
connection.o: /usr/include/bits/wchar.h /usr/include/gconv.h
connection.o: /usr/include/bits/stdio_lim.h /usr/include/sys/time.h
connection.o: /usr/include/time.h /usr/include/sys/select.h
connection.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
connection.o: /usr/include/bits/time.h /usr/include/netinet/in.h
connection.o: /usr/include/limits.h /usr/include/bits/wordsize.h
connection.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
connection.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
connection.o: /usr/include/stdint.h /usr/include/sys/types.h
connection.o: /usr/include/endian.h /usr/include/bits/endian.h
connection.o: /usr/include/sys/sysmacros.h /usr/include/bits/socket.h
connection.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
connection.o: /usr/include/asm/sockios.h /usr/include/bits/in.h
connection.o: /usr/include/bits/byteswap.h lat.h utils.h session.h
connection.o: serversession.h clientsession.h lloginsession.h portsession.h
connection.o: connection.h circuit.h latcpcircuit.h server.h services.h
connection.o: lat_messages.h dn_endian.h
lat_messages.o: lat_messages.h
latcp.o: /usr/include/sys/types.h /usr/include/features.h
latcp.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
latcp.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
latcp.o: /usr/include/bits/sched.h /usr/include/time.h /usr/include/endian.h
latcp.o: /usr/include/bits/endian.h /usr/include/sys/select.h
latcp.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
latcp.o: /usr/include/sys/sysmacros.h /usr/include/sys/uio.h
latcp.o: /usr/include/bits/uio.h /usr/include/sys/socket.h
latcp.o: /usr/include/bits/socket.h /usr/include/limits.h
latcp.o: /usr/include/bits/wordsize.h /usr/include/bits/posix1_lim.h
latcp.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
latcp.o: /usr/include/bits/posix2_lim.h /usr/include/bits/sockaddr.h
latcp.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
latcp.o: /usr/include/sys/un.h /usr/include/string.h /usr/include/sys/ioctl.h
latcp.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
latcp.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
latcp.o: /usr/include/sys/ttydefaults.h /usr/include/sys/wait.h
latcp.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
latcp.o: /usr/include/bits/siginfo.h /usr/include/sys/stat.h
latcp.o: /usr/include/bits/stat.h /usr/include/sys/time.h
latcp.o: /usr/include/bits/time.h /usr/include/sys/utsname.h
latcp.o: /usr/include/bits/utsname.h /usr/include/stdio.h
latcp.o: /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h
latcp.o: /usr/include/bits/wchar.h /usr/include/gconv.h
latcp.o: /usr/include/bits/stdio_lim.h /usr/include/errno.h
latcp.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
latcp.o: /usr/include/asm/errno.h /usr/include/unistd.h
latcp.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
latcp.o: /usr/include/getopt.h /usr/include/signal.h
latcp.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
latcp.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
latcp.o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
latcp.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/syslog.h
latcp.o: /usr/include/sys/syslog.h /usr/include/ctype.h /usr/include/regex.h
latcp.o: /usr/include/stdlib.h /usr/include/alloca.h /usr/include/utmp.h
latcp.o: /usr/include/bits/utmp.h /usr/include/paths.h /usr/include/assert.h
latcp.o: lat.h latcp.h utils.h dn_endian.h
latcpcircuit.o: /usr/include/sys/types.h /usr/include/features.h
latcpcircuit.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
latcpcircuit.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
latcpcircuit.o: /usr/include/bits/sched.h /usr/include/time.h
latcpcircuit.o: /usr/include/endian.h /usr/include/bits/endian.h
latcpcircuit.o: /usr/include/sys/select.h /usr/include/bits/select.h
latcpcircuit.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
latcpcircuit.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
latcpcircuit.o: /usr/include/bits/confname.h /usr/include/getopt.h lat.h
latcpcircuit.o: latcp.h utils.h services.h session.h connection.h circuit.h
latcpcircuit.o: /usr/include/stdio.h /usr/include/libio.h
latcpcircuit.o: /usr/include/_G_config.h /usr/include/wchar.h
latcpcircuit.o: /usr/include/bits/wchar.h /usr/include/gconv.h
latcpcircuit.o: /usr/include/bits/stdio_lim.h latcpcircuit.h server.h
llogin.o: /usr/include/sys/types.h /usr/include/features.h
llogin.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
llogin.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
llogin.o: /usr/include/bits/sched.h /usr/include/time.h /usr/include/endian.h
llogin.o: /usr/include/bits/endian.h /usr/include/sys/select.h
llogin.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
llogin.o: /usr/include/sys/sysmacros.h /usr/include/sys/uio.h
llogin.o: /usr/include/bits/uio.h /usr/include/sys/socket.h
llogin.o: /usr/include/bits/socket.h /usr/include/limits.h
llogin.o: /usr/include/bits/wordsize.h /usr/include/bits/posix1_lim.h
llogin.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
llogin.o: /usr/include/bits/posix2_lim.h /usr/include/bits/sockaddr.h
llogin.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
llogin.o: /usr/include/sys/un.h /usr/include/string.h
llogin.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
llogin.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
llogin.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
llogin.o: /usr/include/sys/wait.h /usr/include/bits/waitflags.h
llogin.o: /usr/include/bits/waitstatus.h /usr/include/bits/siginfo.h
llogin.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
llogin.o: /usr/include/sys/time.h /usr/include/bits/time.h
llogin.o: /usr/include/sys/utsname.h /usr/include/bits/utsname.h
llogin.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
llogin.o: /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h
llogin.o: /usr/include/bits/stdio_lim.h /usr/include/errno.h
llogin.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
llogin.o: /usr/include/asm/errno.h /usr/include/unistd.h
llogin.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
llogin.o: /usr/include/getopt.h /usr/include/signal.h
llogin.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
llogin.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
llogin.o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
llogin.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
llogin.o: /usr/include/syslog.h /usr/include/sys/syslog.h
llogin.o: /usr/include/ctype.h /usr/include/regex.h /usr/include/stdlib.h
llogin.o: /usr/include/alloca.h /usr/include/utmp.h /usr/include/bits/utmp.h
llogin.o: /usr/include/paths.h /usr/include/assert.h lat.h latcp.h utils.h
llogin.o: dn_endian.h
llogincircuit.o: /usr/include/sys/types.h /usr/include/features.h
llogincircuit.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
llogincircuit.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
llogincircuit.o: /usr/include/bits/sched.h /usr/include/time.h
llogincircuit.o: /usr/include/endian.h /usr/include/bits/endian.h
llogincircuit.o: /usr/include/sys/select.h /usr/include/bits/select.h
llogincircuit.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
llogincircuit.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
llogincircuit.o: /usr/include/bits/confname.h /usr/include/getopt.h
llogincircuit.o: /usr/include/syslog.h /usr/include/sys/syslog.h lat.h
llogincircuit.o: latcp.h utils.h services.h session.h connection.h circuit.h
llogincircuit.o: /usr/include/stdio.h /usr/include/libio.h
llogincircuit.o: /usr/include/_G_config.h /usr/include/wchar.h
llogincircuit.o: /usr/include/bits/wchar.h /usr/include/gconv.h
llogincircuit.o: /usr/include/bits/stdio_lim.h latcpcircuit.h llogincircuit.h
llogincircuit.o: server.h
lloginsession.o: /usr/include/sys/types.h /usr/include/features.h
lloginsession.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
lloginsession.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
lloginsession.o: /usr/include/bits/sched.h /usr/include/time.h
lloginsession.o: /usr/include/endian.h /usr/include/bits/endian.h
lloginsession.o: /usr/include/sys/select.h /usr/include/bits/select.h
lloginsession.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
lloginsession.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
lloginsession.o: /usr/include/stdio.h /usr/include/libio.h
lloginsession.o: /usr/include/_G_config.h /usr/include/wchar.h
lloginsession.o: /usr/include/bits/wchar.h /usr/include/gconv.h
lloginsession.o: /usr/include/bits/stdio_lim.h /usr/include/fcntl.h
lloginsession.o: /usr/include/bits/fcntl.h /usr/include/unistd.h
lloginsession.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
lloginsession.o: /usr/include/getopt.h /usr/include/termios.h
lloginsession.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
lloginsession.o: lat.h utils.h session.h connection.h circuit.h
lloginsession.o: latcpcircuit.h server.h clientsession.h lloginsession.h
lloginsession.o: lat_messages.h
main.o: /usr/include/sys/types.h /usr/include/features.h
main.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
main.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
main.o: /usr/include/bits/sched.h /usr/include/time.h /usr/include/endian.h
main.o: /usr/include/bits/endian.h /usr/include/sys/select.h
main.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
main.o: /usr/include/sys/sysmacros.h /usr/include/sys/uio.h
main.o: /usr/include/bits/uio.h /usr/include/stdio.h /usr/include/libio.h
main.o: /usr/include/_G_config.h /usr/include/wchar.h
main.o: /usr/include/bits/wchar.h /usr/include/gconv.h
main.o: /usr/include/bits/stdio_lim.h /usr/include/errno.h
main.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
main.o: /usr/include/asm/errno.h /usr/include/unistd.h
main.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
main.o: /usr/include/getopt.h /usr/include/signal.h
main.o: /usr/include/bits/signum.h /usr/include/bits/siginfo.h
main.o: /usr/include/bits/wordsize.h /usr/include/bits/sigaction.h
main.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
main.o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
main.o: /usr/include/string.h /usr/include/fcntl.h /usr/include/bits/fcntl.h
main.o: /usr/include/syslog.h /usr/include/sys/syslog.h /usr/include/ctype.h
main.o: /usr/include/regex.h /usr/include/stdlib.h /usr/include/alloca.h
main.o: /usr/include/utmp.h /usr/include/bits/utmp.h /usr/include/paths.h
main.o: /usr/include/sys/time.h /usr/include/bits/time.h
main.o: /usr/include/sys/socket.h /usr/include/bits/socket.h
main.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
main.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
main.o: /usr/include/bits/posix2_lim.h /usr/include/bits/sockaddr.h
main.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
main.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
main.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
main.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
main.o: /usr/include/sys/wait.h /usr/include/bits/waitflags.h
main.o: /usr/include/bits/waitstatus.h /usr/include/sys/stat.h
main.o: /usr/include/bits/stat.h /usr/include/sys/utsname.h
main.o: /usr/include/bits/utsname.h /usr/include/netinet/in.h
main.o: /usr/include/stdint.h /usr/include/bits/in.h
main.o: /usr/include/bits/byteswap.h /usr/include/asm/types.h
main.o: /usr/include/linux/if.h /usr/include/linux/types.h
main.o: /usr/include/linux/posix_types.h /usr/include/linux/stddef.h
main.o: /usr/include/asm/posix_types.h /usr/include/linux/socket.h
main.o: /usr/include/linux/if_packet.h /usr/include/linux/if_ether.h lat.h
main.o: utils.h session.h connection.h circuit.h latcpcircuit.h server.h
main.o: services.h
portsession.o: /usr/include/sys/types.h /usr/include/features.h
portsession.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
portsession.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
portsession.o: /usr/include/bits/sched.h /usr/include/time.h
portsession.o: /usr/include/endian.h /usr/include/bits/endian.h
portsession.o: /usr/include/sys/select.h /usr/include/bits/select.h
portsession.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
portsession.o: /usr/include/sys/signal.h /usr/include/signal.h
portsession.o: /usr/include/bits/signum.h /usr/include/bits/siginfo.h
portsession.o: /usr/include/bits/wordsize.h /usr/include/bits/sigaction.h
portsession.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
portsession.o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
portsession.o: /usr/include/stdio.h /usr/include/libio.h
portsession.o: /usr/include/_G_config.h /usr/include/wchar.h
portsession.o: /usr/include/bits/wchar.h /usr/include/gconv.h
portsession.o: /usr/include/bits/stdio_lim.h /usr/include/syslog.h
portsession.o: /usr/include/sys/syslog.h /usr/include/unistd.h
portsession.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
portsession.o: /usr/include/getopt.h /usr/include/fcntl.h
portsession.o: /usr/include/bits/fcntl.h /usr/include/termios.h
portsession.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
portsession.o: lat.h utils.h session.h connection.h circuit.h latcpcircuit.h
portsession.o: server.h serversession.h clientsession.h portsession.h
server.o: /usr/include/sys/types.h /usr/include/features.h
server.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
server.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
server.o: /usr/include/bits/sched.h /usr/include/time.h /usr/include/endian.h
server.o: /usr/include/bits/endian.h /usr/include/sys/select.h
server.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
server.o: /usr/include/sys/sysmacros.h /usr/include/sys/uio.h
server.o: /usr/include/bits/uio.h /usr/include/sys/socket.h
server.o: /usr/include/bits/socket.h /usr/include/limits.h
server.o: /usr/include/bits/wordsize.h /usr/include/bits/posix1_lim.h
server.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
server.o: /usr/include/bits/posix2_lim.h /usr/include/bits/sockaddr.h
server.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
server.o: /usr/include/sys/un.h /usr/include/string.h
server.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
server.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
server.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
server.o: /usr/include/sys/wait.h /usr/include/bits/waitflags.h
server.o: /usr/include/bits/waitstatus.h /usr/include/bits/siginfo.h
server.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
server.o: /usr/include/sys/time.h /usr/include/bits/time.h
server.o: /usr/include/sys/utsname.h /usr/include/bits/utsname.h
server.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
server.o: /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h
server.o: /usr/include/bits/stdio_lim.h /usr/include/errno.h
server.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
server.o: /usr/include/asm/errno.h /usr/include/unistd.h
server.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
server.o: /usr/include/getopt.h /usr/include/signal.h
server.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
server.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
server.o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
server.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
server.o: /usr/include/dirent.h /usr/include/bits/dirent.h
server.o: /usr/include/syslog.h /usr/include/sys/syslog.h
server.o: /usr/include/ctype.h /usr/include/regex.h /usr/include/stdlib.h
server.o: /usr/include/alloca.h /usr/include/utmp.h /usr/include/bits/utmp.h
server.o: /usr/include/paths.h /usr/include/grp.h /usr/include/assert.h
server.o: /usr/include/netinet/in.h /usr/include/stdint.h
server.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
server.o: /usr/include/asm/types.h /usr/include/linux/if.h
server.o: /usr/include/linux/types.h /usr/include/linux/posix_types.h
server.o: /usr/include/linux/stddef.h /usr/include/asm/posix_types.h
server.o: /usr/include/linux/socket.h /usr/include/linux/if_arp.h
server.o: /usr/include/linux/netdevice.h /usr/include/linux/if_ether.h
server.o: /usr/include/linux/if_packet.h /usr/include/asm/atomic.h
server.o: /usr/include/linux/config.h /usr/include/linux/autoconf.h
server.o: /usr/include/asm/cache.h /usr/include/asm/byteorder.h
server.o: /usr/include/linux/byteorder/little_endian.h
server.o: /usr/include/linux/byteorder/swab.h
server.o: /usr/include/linux/byteorder/generic.h lat.h utils.h session.h
server.o: connection.h circuit.h latcpcircuit.h llogincircuit.h server.h
server.o: services.h lat_messages.h dn_endian.h
serversession.o: /usr/include/sys/types.h /usr/include/features.h
serversession.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
serversession.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
serversession.o: /usr/include/bits/sched.h /usr/include/time.h
serversession.o: /usr/include/endian.h /usr/include/bits/endian.h
serversession.o: /usr/include/sys/select.h /usr/include/bits/select.h
serversession.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
serversession.o: /usr/include/stdio.h /usr/include/libio.h
serversession.o: /usr/include/_G_config.h /usr/include/wchar.h
serversession.o: /usr/include/bits/wchar.h /usr/include/gconv.h
serversession.o: /usr/include/bits/stdio_lim.h /usr/include/syslog.h
serversession.o: /usr/include/sys/syslog.h /usr/include/unistd.h
serversession.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
serversession.o: /usr/include/getopt.h /usr/include/fcntl.h
serversession.o: /usr/include/bits/fcntl.h /usr/include/termios.h
serversession.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
serversession.o: lat.h utils.h session.h connection.h circuit.h
serversession.o: latcpcircuit.h server.h serversession.h
services.o: /usr/include/stdio.h /usr/include/features.h
services.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
services.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
services.o: /usr/include/bits/sched.h /usr/include/libio.h
services.o: /usr/include/_G_config.h /usr/include/wchar.h
services.o: /usr/include/bits/wchar.h /usr/include/gconv.h
services.o: /usr/include/bits/stdio_lim.h /usr/include/sys/time.h
services.o: /usr/include/time.h /usr/include/sys/select.h
services.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
services.o: /usr/include/bits/time.h /usr/include/netinet/in.h
services.o: /usr/include/limits.h /usr/include/bits/wordsize.h
services.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
services.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
services.o: /usr/include/stdint.h /usr/include/sys/types.h
services.o: /usr/include/endian.h /usr/include/bits/endian.h
services.o: /usr/include/sys/sysmacros.h /usr/include/bits/socket.h
services.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
services.o: /usr/include/asm/sockios.h /usr/include/bits/in.h
services.o: /usr/include/bits/byteswap.h lat.h utils.h services.h dn_endian.h
session.o: /usr/include/sys/socket.h /usr/include/features.h
session.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
session.o: /usr/include/bits/socket.h /usr/include/limits.h
session.o: /usr/include/bits/wordsize.h /usr/include/bits/posix1_lim.h
session.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
session.o: /usr/include/bits/posix2_lim.h /usr/include/sys/types.h
session.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
session.o: /usr/include/bits/sched.h /usr/include/time.h
session.o: /usr/include/endian.h /usr/include/bits/endian.h
session.o: /usr/include/sys/select.h /usr/include/bits/select.h
session.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
session.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
session.o: /usr/include/asm/sockios.h /usr/include/sys/ioctl.h
session.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
session.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
session.o: /usr/include/sys/ttydefaults.h /usr/include/sys/wait.h
session.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
session.o: /usr/include/bits/siginfo.h /usr/include/sys/stat.h
session.o: /usr/include/bits/stat.h /usr/include/sys/time.h
session.o: /usr/include/bits/time.h /usr/include/stdio.h /usr/include/libio.h
session.o: /usr/include/_G_config.h /usr/include/wchar.h
session.o: /usr/include/bits/wchar.h /usr/include/gconv.h
session.o: /usr/include/bits/stdio_lim.h /usr/include/errno.h
session.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
session.o: /usr/include/asm/errno.h /usr/include/unistd.h
session.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
session.o: /usr/include/getopt.h /usr/include/signal.h
session.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
session.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
session.o: /usr/include/bits/sigstack.h /usr/include/bits/sigthread.h
session.o: /usr/include/string.h /usr/include/fcntl.h
session.o: /usr/include/bits/fcntl.h /usr/include/syslog.h
session.o: /usr/include/sys/syslog.h /usr/include/ctype.h
session.o: /usr/include/regex.h /usr/include/stdlib.h /usr/include/alloca.h
session.o: /usr/include/utmp.h /usr/include/bits/utmp.h /usr/include/paths.h
session.o: /usr/include/pwd.h /usr/include/pty.h /usr/include/termios.h
session.o: /usr/include/bits/termios.h /usr/include/assert.h lat.h utils.h
session.o: session.h connection.h circuit.h latcpcircuit.h server.h
utils.o: /usr/include/sys/types.h /usr/include/features.h
utils.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
utils.o: /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h
utils.o: /usr/include/bits/sched.h /usr/include/time.h /usr/include/endian.h
utils.o: /usr/include/bits/endian.h /usr/include/sys/select.h
utils.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
utils.o: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
utils.o: /usr/include/bits/stat.h /usr/include/sys/fcntl.h
utils.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/string.h
utils.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
utils.o: /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h
utils.o: /usr/include/bits/stdio_lim.h utils.h
