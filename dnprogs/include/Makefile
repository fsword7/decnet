# Makefile for includes

include ../Makefile.common

all: 

install:
	if [ -L /usr/include/netdnet ]; then rm -f /usr/include/netdnet; fi
	install -d $(libprefix)/include/netdnet
	install -m 0644 netdnet/dn.h $(libprefix)/include/netdnet
	install -m 0644 netdnet/dnetdb.h $(libprefix)/include/netdnet

dep depend:	

clean:
	rm -f dnetdb/dn.h

# DO NOT DELETE
