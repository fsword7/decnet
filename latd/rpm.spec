Summary: LAT daemon
Name: %%PACKAGENAME%%
Version: %%VERSION%%
Release: 1
Vendor: Patrick Caulfield
Copyright: GPL
Group: Networking/Utilities
Source: http://download.sourceforge.net/linux-decnet/%%PACKAGENAME%%-%%VERSION%%.tar.gz
%description

LAT implementation for Linux

%files
%%PREFIX%%/sbin/latcp
%%PREFIX%%/sbin/latd
%%PREFIX%%/sbin/moprc
%%PREFIX%%/bin/llogin
%%PREFIX%%/man/man1/llogin.1
%%PREFIX%%/man/man5/latd.conf.5
%%PREFIX%%/man/man8/latcp.8
%%PREFIX%%/man/man8/moprc.8
%%PREFIX%%/man/man8/latd.8
/etc/latd.conf
/etc/rc.d/init.d/lat
/etc/rc.d/rc3.d/S79lat
/etc/rc.d/rc3.d/K79lat


%doc README NEWS lat.html


%post


