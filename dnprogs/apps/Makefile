#
# Makefile for Eduardo's apps
#
include ../Makefile.common
CC        = gcc
INCDIR	  = -I.
LDFLAGS   = 
CFLAGS   += -D_SVID_SOURCE
#------------------------------------------------------------------------------

PROG1OBJS = startnet.o
PROG2OBJS = sethost.o 
PROG4OBJS = dnping.o
PROG5OBJS = ctermd.o
PROG6OBJS = dnmount.o
PROG7OBJS = rmtermd.o

PROG1 = startnet
PROG2 = sethost
PROG4 = dnping
PROG5 = ctermd
PROG6 = dnmount
PROG7 = rmtermd
MANPAGES1 = sethost.1 dnping.1 dnmount.1
MANPAGES5 = decnet.conf.5
MANPAGES8 = startnet.8 ctermd.8 rmtermd.8

ALLPROGS=$(PROG1) $(PROG2) $(PROG4) $(PROG5) $(PROG7) 
ifeq (/usr/include/linux/dap_fs.h,$(wildcard /usr/include/linux/dap_fs.h))
ALLPROGS+=$(PROG6)
endif

all: $(ALLPROGS)

$(PROG1): $(PROG1OBJS) $(DEPLIBDNET)
	$(CC) -o $@ $(CFLAGS) $(PROG1OBJS) $(LIBDNET)

$(PROG2): $(PROG2OBJS) $(DEPLIBDNET)
	$(CC) -o $@ $(CFLAGS) $(PROG2OBJS) $(LIBDNET)

$(PROG3): $(PROG3OBJS)
	$(CC) -o $@ $(CFLAGS) $(PROG3OBJS)

$(PROG4): $(PROG4OBJS) $(DEPLIBDNET)
	$(CC) -o $@ $(CFLAGS) $(PROG4OBJS) $(LIBDNET)

$(PROG5): $(PROG5OBJS) $(DEPLIBDNET) $(DEPLIBDAEMON)
	$(CC) -o $@ $(CFLAGS) $(PROG5OBJS) $(LIBDAEMON) $(LIBDNET) $(PTSLIBS)

$(PROG6): $(PROG6OBJS) $(DEPLIBDNET)
	$(CC) -o $@ $(CFLAGS) $(PROG6OBJS) $(LIBDNET)

$(PROG7): $(PROG7OBJS) $(DEPLIBDNET) $(DEPLIBDAEMON)
	$(CC) -o $@ $(CFLAGS) $(PROG7OBJS) $(LIBDAEMON) $(LIBDNET) $(PTSLIBS)

dep:	
	makedepend *.c 2>/dev/null

clean:
	rm -f $(PROG1) $(PROG2) $(PROG4) $(PROG5) $(PROG6) $(PROG7) \
	*.o *.a *.so *~

install:
	install -d $(prefix)/sbin
	install -d $(prefix)/bin
	install -d $(prefix)/man/man1
	install -d $(prefix)/man/man5
	install -d $(prefix)/man/man8
	install -d $(sysconfprefix)/etc
	install -m 0750 -s $(PROG1) $(prefix)/sbin
	install -m 0755 -s $(PROG2) $(prefix)/bin
	install -m 0755 -s $(PROG4) $(prefix)/bin
	install -m 0755 -s $(PROG5) $(prefix)/sbin
	if [ -f $(PROG6) ]; then install -m 0750 -s $(PROG6) $(prefix)/sbin; fi
	install -m 0755 -s $(PROG7) $(prefix)/sbin
	install -m 0644 $(MANPAGES1) $(prefix)/man/man1
	install -m 0644 $(MANPAGES5) $(prefix)/man/man5
	install -m 0644 $(MANPAGES8) $(prefix)/man/man8
	if [ ! -f $(sysconfprefix)/etc/decnet.conf ]; then \
		install -m 0644 decnet.conf $(sysconfprefix)/etc/decnet.conf.sample; \
	fi

# DO NOT DELETE THIS LINE -- make  depend  depends  on it.
