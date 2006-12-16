Summary: DECnet tools and libraries
Name: %%PACKAGENAME%%
Version: %%VERSION%%
Release: 1
Vendor: Patrick Caulfield and The Linux DECnet Project team
License: GPL
Group: Networking/Utilities
Url:	http://linux-decnet.sourceforge.net
Source: http://download.sourceforge.net/linux-decnet/%%PACKAGENAME%%-%%VERSION%%.tar.gz
%description
DECnet programs for Linux.

These tools are the application layer interface for DECnet on Linux systems.
They provide file/terminal access facilities between OpenVMS and Linux and 
remote execution of commands.

To use them you will need to have DECnet built into your kernel or as a module.

%package devel
Summary: The Libraries and include files for DECnet developement
Group: Networking/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
This package contains the libraries and include files used to develop programs
that use the DECnet protocol

%files
%%PREFIX%%/bin/dncopy
%%PREFIX%%/bin/dntype
%%PREFIX%%/bin/dntask
%%PREFIX%%/bin/dndel
%%PREFIX%%/bin/dndir
%%PREFIX%%/bin/dnprint
%%PREFIX%%/bin/dnsubmit
%%PREFIX%%/bin/sethost
%%PREFIX%%/bin/dnping
%%PREFIX%%/bin/dnlogin
%%PREFIX%%/bin/phone
%%PREFIX%%/sbin/ctermd
%%PREFIX%%/sbin/rmtermd
%%PREFIX%%/sbin/phoned
%%PREFIX%%/sbin/dnetd
%%PREFIX%%/sbin/fal
%%PREFIX%%/sbin/dnroute
%%PREFIX%%/sbin/dnetinfo
%%PREFIX%%/sbin/decnetconf
%%PREFIX%%/sbin/setether
%%PREFIX%%/sbin/sendvmsmail
%%PREFIX%%/sbin/vmsmaild
%%PREFIX%%/sbin/multinet
%%PREFIX%%/share/man/man1/dncopy.1.gz
%%PREFIX%%/share/man/man1/dntype.1.gz
%%PREFIX%%/share/man/man1/phone.1.gz
%%PREFIX%%/share/man/man1/dntask.1.gz
%%PREFIX%%/share/man/man1/dndel.1.gz
%%PREFIX%%/share/man/man1/dndir.1.gz
%%PREFIX%%/share/man/man1/dnprint.1.gz
%%PREFIX%%/share/man/man1/dnsubmit.1.gz
%%PREFIX%%/share/man/man1/sethost.1.gz
%%PREFIX%%/share/man/man1/dnping.1.gz
%%PREFIX%%/share/man/man1/dnlogin.1.gz
%%PREFIX%%/share/man/man5/decnet.conf.5.gz
%%PREFIX%%/share/man/man5/decnet.proxy.5.gz
%%PREFIX%%/share/man/man5/dnetd.conf.5.gz
%%PREFIX%%/share/man/man5/vmsmail.conf.5.gz
%%PREFIX%%/share/man/man8/fal.8.gz
%%PREFIX%%/share/man/man8/dnroute.8.gz
%%PREFIX%%/share/man/man8/dnetinfo.8.gz
%%PREFIX%%/share/man/man8/phoned.8.gz
%%PREFIX%%/share/man/man8/dnetd.8.gz
%%PREFIX%%/share/man/man8/ctermd.8.gz
%%PREFIX%%/share/man/man8/rmtermd.8.gz
%%PREFIX%%/share/man/man8/sendvmsmail.8.gz
%%PREFIX%%/share/man/man8/vmsmaild.8.gz
%%PREFIX%%/share/man/man8/setether.8.gz
%%PREFIX%%/share/man/man8/decnetconf.8.gz
%%PREFIX%%/share/man/man8/multinet.8.gz
/etc/rc.d/init.d/decnet
/etc/decnet.proxy
%%LIBPREFIX%%/lib/libdnet.so.1
%%LIBPREFIX%%/lib/libdnet.so.%%MAJOR_VERSION%%
%%LIBPREFIX%%/lib/libdnet.so.%%VERSION%%
%%LIBPREFIX%%/lib/libdnet_daemon.so.%%MAJOR_VERSION%%
%%LIBPREFIX%%/lib/libdnet_daemon.so.%%VERSION%%
%%LIBPREFIX%%/lib/libdap.so.%%MAJOR_VERSION%%
%%LIBPREFIX%%/lib/libdap.so.%%VERSION%%
%%LIBPREFIX%%/lib/librms.so.%%MAJOR_VERSION%%
%%LIBPREFIX%%/lib/librms.so.%%VERSION%%

%files devel
%%PREFIX%%/share/man/man3/dnet_addr.3.gz
%%PREFIX%%/share/man/man3/dnet_conn.3.gz
%%PREFIX%%/share/man/man3/dnet_htoa.3.gz
%%PREFIX%%/share/man/man3/dnet_ntoa.3.gz
%%PREFIX%%/share/man/man3/dnet_eof.3.gz
%%PREFIX%%/share/man/man3/dnet_accept.3.gz
%%PREFIX%%/share/man/man3/dnet_reject.3.gz
%%PREFIX%%/share/man/man3/dnet_getnode.3.gz
%%PREFIX%%/share/man/man3/dnet_endnode.3.gz
%%PREFIX%%/share/man/man3/dnet_nextnode.3.gz
%%PREFIX%%/share/man/man3/getnodeadd.3.gz
%%PREFIX%%/share/man/man3/getnodebyaddr.3.gz
%%PREFIX%%/share/man/man3/getnodebyname.3.gz
%%PREFIX%%/share/man/man3/libdnet.3.gz
%%PREFIX%%/share/man/man3/setnodeent.3.gz
%%PREFIX%%/share/man/man3/dnet_daemon.3.gz
%%LIBPREFIX%%/lib/libdnet.a
%%LIBPREFIX%%/lib/libdnet.so
%%LIBPREFIX%%/lib/libdnet_daemon.a
%%LIBPREFIX%%/lib/libdnet_daemon.so
%%LIBPREFIX%%/lib/libdap.a
%%LIBPREFIX%%/lib/libdap.so
%%LIBPREFIX%%/lib/librms.a
%%LIBPREFIX%%/lib/librms.so
%%LIBPREFIX%%/lib/libvaxdata.a
/usr/include/netdnet/dnetdb.h
/usr/include/netdnet/dn.h
/usr/include/rms.h
/usr/include/rabdef.h
/usr/include/fabdef.h
/usr/include/convert_vax_data.h



%config %%CONFPREFIX%%/etc/decnet.conf.sample
%%CONFPREFIX%%/etc/dnetd.conf

%dir /usr/include/netdnet

%doc README NEWS fal.README mail.README dnetd.README phone.README librms.README libvaxdata.pdf

%post 
chkconfig --level 345 decnet on
/sbin/ldconfig; if [ ! -f /etc/decnet.conf ]; then %%PREFIX%%/sbin/decnetconf </dev/tty ; fi

%postun
ldconfig

%Changelog

* Thu May 26 2006 Patrick Caulfield <patrick@tykepenguin.com> - 2.37-1
  First release
