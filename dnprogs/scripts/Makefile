# Makefile for scripts
#
# With luck this should cope with Debian, Redhat, SuSE, Caldera & Slackware type
# distributions - but no promises.
#


include ../Makefile.common

all: 


install:
ifndef MAKEDEB
	if [ -d /var/lib/dpkg ]; then \
	  install -Dm 0700 decnet.sh $(DESTDIR)/etc/init.d/decnet.sh; \
	elif [ -d /var/lib/YaST ]; then \
	  install -Dm 0700 decnet.sh $(DESTDIR)/sbin/init.d/decnet; \
	elif [ -d /var/lib/rpm ]; then \
	  install -Dm 0700 decnet.sh $(DESTDIR)/etc/rc.d/init.d/decnet; \
	else install -Dm 0700 rc.decnet $(DESTDIR)/etc/rc.d/rc.decnet; \
	fi
endif
	install -m 0755 setup.sh $(prefix)/sbin/decnetconf
	install -m 0644 decnetconf.8 $(prefix)/man/man8

dep depend:	


clean:


# DO NOT DELETE THIS LINE -- make  depend  depends  on it.
