Summary: DECnet tools and libraries
Name: %%PACKAGENAME%%
Version: %%VERSION%%
Release: 1
Vendor: Patrick Caulfield and The Linux DECnet Project team
Copyright: GPL
Group: Networking/Utilities
Source: http://download.sourceforge.net/linux-decnet/%%PACKAGENAME%%-%%VERSION%%.tar.gz
%description

DECnet programs for Linux.

These tools are the application layer interface for DECnet on Linux systems.
They provide file/terminal access facilities between OpenVMS and Linux and 
remote execution of commands.

To use them you will need to have DECnet built into your kernel.
See http://linux-decnet.sourceforge.net to get the kernel patch and
instructions on how to apply it.

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
%%PREFIX%%/bin/phone
%%PREFIX%%/sbin/ctermd
%%PREFIX%%/sbin/rmtermd
%%PREFIX%%/sbin/phoned
%%PREFIX%%/sbin/dnetd
%%PREFIX%%/sbin/fal
%%PREFIX%%/sbin/startnet
%%PREFIX%%/sbin/decnetconf
%%PREFIX%%/man/man1/dncopy.1
%%PREFIX%%/man/man1/dntype.1
%%PREFIX%%/man/man1/dntask.1
%%PREFIX%%/man/man1/dndel.1
%%PREFIX%%/man/man1/dndir.1
%%PREFIX%%/man/man1/dnprint.1
%%PREFIX%%/man/man1/dnsubmit.1
%%PREFIX%%/man/man1/sethost.1
%%PREFIX%%/man/man1/dnping.1
%%PREFIX%%/man/man3/dnet_addr.3
%%PREFIX%%/man/man3/dnet_conn.3
%%PREFIX%%/man/man3/dnet_htoa.3
%%PREFIX%%/man/man3/dnet_ntoa.3
%%PREFIX%%/man/man3/dnet_getnode.3
%%PREFIX%%/man/man3/dnet_nextnode.3
%%PREFIX%%/man/man3/dnet_endnode.3
%%PREFIX%%/man/man3/getnodeadd.3
%%PREFIX%%/man/man3/getnodebyaddr.3
%%PREFIX%%/man/man3/getnodebyname.3
%%PREFIX%%/man/man3/libdnet.3
%%PREFIX%%/man/man3/setnodeent.3
%%PREFIX%%/man/man3/dnet_daemon.3
%%PREFIX%%/man/man5/decnet.conf.5
%%PREFIX%%/man/man5/decnet.proxy.5
%%PREFIX%%/man/man5/dnetd.conf.5
%%PREFIX%%/man/man5/vmsmail.conf.5
%%PREFIX%%/man/man8/fal.8
%%PREFIX%%/man/man8/phoned.8
%%PREFIX%%/man/man8/dnetd.8
%%PREFIX%%/man/man8/ctermd.8
%%PREFIX%%/man/man8/rmtermd.8
%%PREFIX%%/man/man8/startnet.8
%%PREFIX%%/man/man8/sendvmsmail.8
%%PREFIX%%/man/man8/vmsmaild.8
/etc/rc.d/init.d/decnet
%%LIBPREFIX%%/lib/libdnet.a
%%LIBPREFIX%%/lib/libdnet.so.1
%%LIBPREFIX%%/lib/libdnet.so
%%LIBPREFIX%%/lib/libdnet.so.%%MAJOR_VERSION%%
%%LIBPREFIX%%/lib/libdnet.so.%%VERSION%%
%%LIBPREFIX%%/lib/libdnet_daemon.a
%%LIBPREFIX%%/lib/libdnet_daemon.so
%%LIBPREFIX%%/lib/libdnet_daemon.so.%%MAJOR_VERSION%%
%%LIBPREFIX%%/lib/libdnet_daemon.so.%%VERSION%%
%%LIBPREFIX%%/lib/libdap.a
%%LIBPREFIX%%/lib/libdap.so
%%LIBPREFIX%%/lib/libdap.so.%%MAJOR_VERSION%%
%%LIBPREFIX%%/lib/libdap.so.%%VERSION%%
%%LIBPREFIX%%/lib/librms.a
%%LIBPREFIX%%/lib/librms.so
%%LIBPREFIX%%/lib/librms.so.%%MAJOR_VERSION%%
%%LIBPREFIX%%/lib/librms.so.%%VERSION%%
/usr/include/netdnet/dnetdb.h
/usr/include/netdnet/dn.h
/usr/include/rms.h
/usr/include/rabdef.h
/usr/include/fabdef.h

%config %%CONFPREFIX%%/etc/decnet.conf.sample
%%CONFPREFIX%%/etc/dnetd.conf

%dir /usr/include/netdnet

%doc README NEWS fal.README mail.README dnetd.README phone.README librms.README

%post 
/sbin/ldconfig; if [ ! -f /etc/decnet.conf ]; then %%PREFIX%%/sbin/decnetconf </dev/tty ; fi

