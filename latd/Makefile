# Makefile for LAT server

VERSION=0.6

PROG1=latd
PROG2=latcp
MANPAGES8=latd.8 latcp.8
PROG1OBJS=main.o utils.o server.o connection.o session.o \
	  serversession.o clientsession.o services.o latcpcircuit.o lat_messages.o
PROG2OBJS=latcp.o utils.o

DEFS=-DVERSION=\"$(VERSION)\" -DLATCP_SOCKNAME=\"$(LATCPSOCK)\"
DEFS+=#-DOLDSTUFF -DVERBOSE_DEBUG -DNO_FORK

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

all: $(PROG1)  $(PROG2)

$(PROG1): depend $(PROG1OBJS) 
	$(CXX) -o $@ $(PROG1OBJS) $(LIBDNET) -lutil

$(PROG2): depend $(PROG2OBJS) 
	$(CXX) -o $@ $(PROG2OBJS) $(LIBDNET)

install:
	install -d $(prefix)/bin
	install -d $(prefix)/man/man8
	install -m 0750 -s $(PROG1) $(prefix)/sbin
	install -m 0750 -s $(PROG2) $(prefix)/sbin
#	install -m 0644 $(MANPAGES8) $(prefix)/man/man8

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
		latd/BUGS latd/README.PLEASE latd/NEWS latd/Makefile \
                latd/WARRANTY latd/COPYING latd/INSTALL latd/TODO

# DO NOT DELETE THIS LINE -- make  depend  depends  on it.

clientsession.o: /usr/include/sys/types.h /usr/include/features.h
clientsession.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
clientsession.o: /usr/include/bits/types.h
clientsession.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stddef.h
clientsession.o: /usr/include/time.h /usr/include/endian.h
clientsession.o: /usr/include/bits/endian.h /usr/include/sys/select.h
clientsession.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
clientsession.o: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
clientsession.o: /usr/include/bits/stat.h /usr/include/stdio.h
clientsession.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stdarg.h
clientsession.o: /usr/include/libio.h /usr/include/_G_config.h
clientsession.o: /usr/include/bits/stdio_lim.h /usr/include/fcntl.h
clientsession.o: /usr/include/bits/fcntl.h /usr/include/unistd.h
clientsession.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
clientsession.o: /usr/include/getopt.h lat.h utils.h /usr/include/pty.h
clientsession.o: /usr/include/termios.h /usr/include/bits/termios.h
clientsession.o: /usr/include/sys/ttydefaults.h /usr/include/sys/ioctl.h
clientsession.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
clientsession.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
clientsession.o: session.h connection.h latcpcircuit.h server.h
clientsession.o: clientsession.h lat_messages.h
connection.o: /usr/include/stdio.h /usr/include/features.h
connection.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
connection.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stddef.h
connection.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stdarg.h
connection.o: /usr/include/bits/types.h /usr/include/libio.h
connection.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
connection.o: /usr/include/sys/time.h /usr/include/time.h
connection.o: /usr/include/sys/select.h /usr/include/bits/select.h
connection.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
connection.o: /usr/include/netinet/in.h /usr/include/limits.h
connection.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
connection.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
connection.o: /usr/include/stdint.h /usr/include/bits/wordsize.h
connection.o: /usr/include/sys/types.h /usr/include/endian.h
connection.o: /usr/include/bits/endian.h /usr/include/sys/sysmacros.h
connection.o: /usr/include/bits/socket.h /usr/include/bits/sockaddr.h
connection.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
connection.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h lat.h
connection.o: utils.h /usr/include/pty.h /usr/include/termios.h
connection.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
connection.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
connection.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
connection.o: /usr/include/bits/ioctl-types.h session.h serversession.h
connection.o: clientsession.h connection.h latcpcircuit.h server.h services.h
connection.o: lat_messages.h dn_endian.h
lat_messages.o: lat_messages.h
latcp.o: /usr/include/sys/types.h /usr/include/features.h
latcp.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
latcp.o: /usr/include/bits/types.h
latcp.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stddef.h
latcp.o: /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h
latcp.o: /usr/include/sys/select.h /usr/include/bits/select.h
latcp.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
latcp.o: /usr/include/sys/uio.h /usr/include/bits/uio.h
latcp.o: /usr/include/sys/socket.h /usr/include/bits/socket.h
latcp.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
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
latcp.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stdarg.h
latcp.o: /usr/include/libio.h /usr/include/_G_config.h
latcp.o: /usr/include/bits/stdio_lim.h /usr/include/errno.h
latcp.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
latcp.o: /usr/include/asm/errno.h /usr/include/unistd.h
latcp.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
latcp.o: /usr/include/getopt.h /usr/include/signal.h
latcp.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
latcp.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
latcp.o: /usr/include/bits/sigstack.h /usr/include/fcntl.h
latcp.o: /usr/include/bits/fcntl.h /usr/include/syslog.h
latcp.o: /usr/include/sys/syslog.h /usr/include/ctype.h /usr/include/regex.h
latcp.o: /usr/include/stdlib.h /usr/include/alloca.h /usr/include/utmp.h
latcp.o: /usr/include/bits/utmp.h /usr/include/paths.h /usr/include/assert.h
latcp.o: latcp.h utils.h /usr/include/pty.h /usr/include/termios.h
latcp.o: /usr/include/bits/termios.h dn_endian.h
latcpcircuit.o: /usr/include/sys/types.h /usr/include/features.h
latcpcircuit.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
latcpcircuit.o: /usr/include/bits/types.h
latcpcircuit.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stddef.h
latcpcircuit.o: /usr/include/time.h /usr/include/endian.h
latcpcircuit.o: /usr/include/bits/endian.h /usr/include/sys/select.h
latcpcircuit.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
latcpcircuit.o: /usr/include/sys/sysmacros.h /usr/include/unistd.h
latcpcircuit.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
latcpcircuit.o: /usr/include/getopt.h lat.h latcp.h utils.h
latcpcircuit.o: /usr/include/pty.h /usr/include/termios.h
latcpcircuit.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
latcpcircuit.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
latcpcircuit.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
latcpcircuit.o: /usr/include/bits/ioctl-types.h services.h session.h
latcpcircuit.o: connection.h latcpcircuit.h server.h
main.o: /usr/include/sys/types.h /usr/include/features.h
main.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
main.o: /usr/include/bits/types.h
main.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stddef.h
main.o: /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h
main.o: /usr/include/sys/select.h /usr/include/bits/select.h
main.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
main.o: /usr/include/sys/uio.h /usr/include/bits/uio.h /usr/include/stdio.h
main.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stdarg.h
main.o: /usr/include/libio.h /usr/include/_G_config.h
main.o: /usr/include/bits/stdio_lim.h /usr/include/errno.h
main.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
main.o: /usr/include/asm/errno.h /usr/include/unistd.h
main.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
main.o: /usr/include/getopt.h /usr/include/signal.h
main.o: /usr/include/bits/signum.h /usr/include/bits/siginfo.h
main.o: /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h
main.o: /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h
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
main.o: /usr/include/stdint.h /usr/include/bits/wordsize.h
main.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
main.o: /usr/include/asm/types.h /usr/include/linux/if.h
main.o: /usr/include/linux/types.h /usr/include/linux/posix_types.h
main.o: /usr/include/linux/stddef.h /usr/include/asm/posix_types.h
main.o: /usr/include/linux/socket.h /usr/include/linux/if_packet.h
main.o: /usr/include/linux/if_ether.h lat.h utils.h /usr/include/pty.h
main.o: /usr/include/termios.h /usr/include/bits/termios.h session.h
main.o: connection.h latcpcircuit.h server.h services.h
server.o: /usr/include/sys/types.h /usr/include/features.h
server.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
server.o: /usr/include/bits/types.h
server.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stddef.h
server.o: /usr/include/time.h /usr/include/endian.h
server.o: /usr/include/bits/endian.h /usr/include/sys/select.h
server.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
server.o: /usr/include/sys/sysmacros.h /usr/include/sys/uio.h
server.o: /usr/include/bits/uio.h /usr/include/sys/socket.h
server.o: /usr/include/bits/socket.h /usr/include/limits.h
server.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
server.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
server.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
server.o: /usr/include/asm/sockios.h /usr/include/sys/un.h
server.o: /usr/include/string.h /usr/include/sys/ioctl.h
server.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
server.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
server.o: /usr/include/sys/ttydefaults.h /usr/include/sys/wait.h
server.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
server.o: /usr/include/bits/siginfo.h /usr/include/sys/stat.h
server.o: /usr/include/bits/stat.h /usr/include/sys/time.h
server.o: /usr/include/bits/time.h /usr/include/sys/utsname.h
server.o: /usr/include/bits/utsname.h /usr/include/stdio.h
server.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stdarg.h
server.o: /usr/include/libio.h /usr/include/_G_config.h
server.o: /usr/include/bits/stdio_lim.h /usr/include/errno.h
server.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
server.o: /usr/include/asm/errno.h /usr/include/unistd.h
server.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
server.o: /usr/include/getopt.h /usr/include/signal.h
server.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
server.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
server.o: /usr/include/bits/sigstack.h /usr/include/fcntl.h
server.o: /usr/include/bits/fcntl.h /usr/include/syslog.h
server.o: /usr/include/sys/syslog.h /usr/include/ctype.h /usr/include/regex.h
server.o: /usr/include/stdlib.h /usr/include/alloca.h /usr/include/utmp.h
server.o: /usr/include/bits/utmp.h /usr/include/paths.h /usr/include/assert.h
server.o: /usr/include/netinet/in.h /usr/include/stdint.h
server.o: /usr/include/bits/wordsize.h /usr/include/bits/in.h
server.o: /usr/include/bits/byteswap.h /usr/include/asm/types.h
server.o: /usr/include/linux/if.h /usr/include/linux/types.h
server.o: /usr/include/linux/posix_types.h /usr/include/linux/stddef.h
server.o: /usr/include/asm/posix_types.h /usr/include/linux/socket.h
server.o: /usr/include/linux/if_packet.h /usr/include/linux/if_ether.h lat.h
server.o: utils.h /usr/include/pty.h /usr/include/termios.h
server.o: /usr/include/bits/termios.h session.h connection.h latcpcircuit.h
server.o: server.h services.h lat_messages.h dn_endian.h
serversession.o: /usr/include/sys/types.h /usr/include/features.h
serversession.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
serversession.o: /usr/include/bits/types.h
serversession.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stddef.h
serversession.o: /usr/include/time.h /usr/include/endian.h
serversession.o: /usr/include/bits/endian.h /usr/include/sys/select.h
serversession.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
serversession.o: /usr/include/sys/sysmacros.h /usr/include/stdio.h
serversession.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stdarg.h
serversession.o: /usr/include/libio.h /usr/include/_G_config.h
serversession.o: /usr/include/bits/stdio_lim.h /usr/include/unistd.h
serversession.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
serversession.o: /usr/include/getopt.h /usr/include/fcntl.h
serversession.o: /usr/include/bits/fcntl.h /usr/include/termios.h
serversession.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
serversession.o: lat.h utils.h /usr/include/pty.h /usr/include/sys/ioctl.h
serversession.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
serversession.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
serversession.o: session.h connection.h latcpcircuit.h server.h
serversession.o: serversession.h
services.o: /usr/include/stdio.h /usr/include/features.h
services.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
services.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stddef.h
services.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stdarg.h
services.o: /usr/include/bits/types.h /usr/include/libio.h
services.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
services.o: /usr/include/sys/time.h /usr/include/time.h
services.o: /usr/include/sys/select.h /usr/include/bits/select.h
services.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
services.o: /usr/include/netinet/in.h /usr/include/limits.h
services.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
services.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
services.o: /usr/include/stdint.h /usr/include/bits/wordsize.h
services.o: /usr/include/sys/types.h /usr/include/endian.h
services.o: /usr/include/bits/endian.h /usr/include/sys/sysmacros.h
services.o: /usr/include/bits/socket.h /usr/include/bits/sockaddr.h
services.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
services.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h lat.h utils.h
services.o: /usr/include/pty.h /usr/include/termios.h
services.o: /usr/include/bits/termios.h /usr/include/sys/ttydefaults.h
services.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
services.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
services.o: /usr/include/bits/ioctl-types.h services.h dn_endian.h
session.o: /usr/include/sys/socket.h /usr/include/features.h
session.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
session.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stddef.h
session.o: /usr/include/bits/socket.h /usr/include/limits.h
session.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
session.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
session.o: /usr/include/sys/types.h /usr/include/bits/types.h
session.o: /usr/include/time.h /usr/include/endian.h
session.o: /usr/include/bits/endian.h /usr/include/sys/select.h
session.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
session.o: /usr/include/sys/sysmacros.h /usr/include/bits/sockaddr.h
session.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
session.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
session.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
session.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
session.o: /usr/include/sys/wait.h /usr/include/bits/waitflags.h
session.o: /usr/include/bits/waitstatus.h /usr/include/bits/siginfo.h
session.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
session.o: /usr/include/sys/time.h /usr/include/bits/time.h
session.o: /usr/include/stdio.h
session.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stdarg.h
session.o: /usr/include/libio.h /usr/include/_G_config.h
session.o: /usr/include/bits/stdio_lim.h /usr/include/errno.h
session.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
session.o: /usr/include/asm/errno.h /usr/include/unistd.h
session.o: /usr/include/bits/posix_opt.h /usr/include/bits/confname.h
session.o: /usr/include/getopt.h /usr/include/signal.h
session.o: /usr/include/bits/signum.h /usr/include/bits/sigaction.h
session.o: /usr/include/bits/sigcontext.h /usr/include/asm/sigcontext.h
session.o: /usr/include/bits/sigstack.h /usr/include/string.h
session.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
session.o: /usr/include/syslog.h /usr/include/sys/syslog.h
session.o: /usr/include/ctype.h /usr/include/regex.h /usr/include/stdlib.h
session.o: /usr/include/alloca.h /usr/include/utmp.h /usr/include/bits/utmp.h
session.o: /usr/include/paths.h /usr/include/pwd.h /usr/include/pty.h
session.o: /usr/include/termios.h /usr/include/bits/termios.h
session.o: /usr/include/assert.h lat.h utils.h session.h connection.h
session.o: latcpcircuit.h server.h
utils.o: /usr/include/sys/types.h /usr/include/features.h
utils.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
utils.o: /usr/include/bits/types.h
utils.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stddef.h
utils.o: /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h
utils.o: /usr/include/sys/select.h /usr/include/bits/select.h
utils.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
utils.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
utils.o: /usr/include/sys/fcntl.h /usr/include/fcntl.h
utils.o: /usr/include/bits/fcntl.h /usr/include/string.h /usr/include/stdio.h
utils.o: /usr/lib/gcc-lib/i386-linux/2.95.2/include/stdarg.h
utils.o: /usr/include/libio.h /usr/include/_G_config.h
utils.o: /usr/include/bits/stdio_lim.h utils.h /usr/include/pty.h
utils.o: /usr/include/termios.h /usr/include/bits/termios.h
utils.o: /usr/include/sys/ttydefaults.h /usr/include/sys/ioctl.h
utils.o: /usr/include/bits/ioctls.h /usr/include/asm/ioctls.h
utils.o: /usr/include/asm/ioctl.h /usr/include/bits/ioctl-types.h
