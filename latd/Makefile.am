## Makefile.am for LAT server

AUTOMAKE_OPTIONS = 1.4 foreign

AM_CXXFLAGS = -fno-rtti -fno-exceptions -Wstrict-prototypes -Wno-deprecated -Wno-unused

INCLUDES = -DLATD_CONF=\"$(sysconfdir)/latd.conf\" -DSBINDIR=\"$(sbindir)\" -DBINDIR=\"$(bindir)\"

sbin_PROGRAMS = latd latcp moprc
bin_PROGRAMS = llogin
sysconf_DATA = latd.conf
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
	utils.h utils.cc \
	dn_endian.h lat.h
latd_LDADD = -lutil
latcp_SOURCES = latcp.h latcp.cc utils.h utils.cc \
	 dn_endian.h lat.h
llogin_SOURCES = llogin.cc utils.cc
moprc_SOURCES = moprc.h moprc.cc interfaces.cc utils.cc
EXTRA_DIST = $(man_MANS) BUGS WARRANTY latd.conf \
	interfaces-linux.cc interfaces-linux.h \
	interfaces-bpf.cc interfaces-bpf.h
EXTRA_latd_SOURCES = @INTERFACE_SOURCE@
EXTRA_moprc_SOURCES = $(EXTRA_latd_SOURCES)

latd_DEPENDANCIES = @INTERFACE_SOURCE@
latd_LDADD = @INTERFACE_SOURCE@ @LIBUTIL@

moprc_DEPENDANCIES = @INTERFACE_SOURCE@
moprc_LDADD = @INTERFACE_SOURCE@

llogin_LDADD = @LIBLOCKDEV@