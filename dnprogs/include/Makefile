# Makefile for includes

include ../Makefile.common

all: 
# Releases are always made for 2.4 kernels
ifdef RELEASE	
	@cp kernel/netdnet/dn.h netdnet || true
endif


install:
	if [ -L $(libprefix)/include/netdnet ]; then \
		rm -f $(libprefix)/include/netdnet; \
	fi
	install -d $(libprefix)/include/netdnet
	install -m 0644 netdnet/dn.h $(libprefix)/include/netdnet
	install -m 0644 netdnet/dnetdb.h $(libprefix)/include/netdnet

dep depend:	

clean:
	rm -f dnetdb/dn.h

# DO NOT DELETE
