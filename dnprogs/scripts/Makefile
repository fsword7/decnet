# Makefile for scripts
#
# With luck this should cope with Debian, Redhat, SuSE and Slackware type
# distributions - but no promises.
#


include ../Makefile.common

all: 


install:
	if [ -d /var/lib/dpkg ]; then \
	  install -m 0700 decnet.sh /etc/init.d/decnet.sh; \
	elif [ -d /var/lib/YaST ]; then \
	  install -m 0700 decnet.sh /sbin/init.d/decnet; \
	elif [ -d /var/lib/rpm ]; then \
	  install -m 0700 decnet.sh /etc/rc.d/init.d/decnet; \
	else install -m 0700 rc.decnet /etc/rc.d/rc.decnet; \
	fi
	install -m 0755 setup.sh $(prefix)/sbin/decnetconf

dep depend:	


clean:


# DO NOT DELETE THIS LINE -- make  depend  depends  on it.
