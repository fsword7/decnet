## Makefile.am for LAT server

AUTOMAKE_OPTIONS = 1.4 foreign

AM_CXXFLAGS = -fno-rtti -fno-exceptions -Wstrict-prototypes -Wno-unused

INCLUDES = -DLATD_CONF=\"$(sysconfdir)/latd.conf\" -DSBINDIR=\"$(sbindir)\" -DBINDIR=\"$(bindir)\"

sbin_PROGRAMS = latd latcp moprc
bin_PROGRAMS = llogin
sysconf_DATA = latd.conf.sample
man_MANS = latd.8 latcp.8 llogin.1 moprc.8 latd.conf.5
latd_SOURCES = main.cc \
	circuit.h circuit.cc \
	clientsession.h clientsession.cc \
	connection.h connection.cc \
	interfaces.h interfaces.cc \
	lat_messages.h lat_messages.cc \
	latcpcircuit.h latcpcircuit.cc \
	llogincircuit.h llogincircuit.cc \
	lloginsession.h lloginsession.cc \
	localportsession.h localportsession.cc \
	queuedsession.h queuedsession.cc \
	localport.h localport.cc \
	server.h server.cc \
	serversession.h serversession.cc \
	services.h services.cc \
	session.h session.cc \
	reversesession.h reversesession.cc \
	utils.h utils.cc \
	dn_endian.h lat.h
latcp_SOURCES = latcp.h latcp.cc utils.h utils.cc \
	 dn_endian.h lat.h
llogin_SOURCES = llogin.cc utils.cc
moprc_SOURCES = moprc.h moprc.cc interfaces.cc utils.cc
EXTRA_DIST = $(man_MANS) WARRANTY latd.conf.sample lat.html \
	interfaces-linux.cc interfaces-linux.h \
	interfaces-bpf.cc interfaces-bpf.h \
	mkrpm.sh rpm.spec startlat.sh

latd_DEPENDENCIES = @INTERFACE@
latd_LDADD = $(latd_DEPENDENCIES) @LIBUTIL@

moprc_DEPENDENCIES = @INTERFACE@
moprc_LDADD = $(moprc_DEPENDENCIES)

llogin_LDADD = @LIBLOCKDEV@

