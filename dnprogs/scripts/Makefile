# Makefile for scripts
#
# With luck this should cope with Debian, Redhat, SuSE and Slackware type
# distributions - but no promises.
#


include ../Makefile.common

all: 


install:
	if [ -d /var/lib/dpkg ]; then \
	  install -Dm 0700 decnet.sh $(DESTDIR)/etc/init.d/decnet.sh; \
	elif [ -d /var/lib/YaST ]; then \
	  install -Dm 0700 decnet.sh $(DESTDIR)/sbin/init.d/decnet; \
	elif [ -d /var/lib/rpm ]; then \
	  install -Dm 0700 decnet.sh $(DESTDIR)/etc/rc.d/init.d/decnet; \
	else install -Dm 0700 rc.decnet $(DESTDIR)/etc/rc.d/rc.decnet; \
	fi
	install -m 0755 setup.sh $(prefix)/sbin/decnetconf

dep depend:	


clean:


# DO NOT DELETE THIS LINE -- make  depend  depends  on it.
