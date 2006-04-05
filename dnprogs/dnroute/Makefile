include ../Makefile.common

DNROUTE=dnroute

CFLAGS	+= -Inetlink/include

all: $(DNROUTE)

$(DNROUTE): get_neigh.c send_route.c routing_msg.c csum.c hash.c netlink/libnetlink.a
	$(CC) $(CFLAGS) -o $@ $^ -L../libdnet -Lnetlink -ldnet -lnetlink

netlink/libnetlink.a:
	$(MAKE) -C netlink

install:
	install -d $(prefix)/sbin
	install -d $(manprefix)/man/man8
	install -m 0755 -s dnroute $(prefix)/sbin
	install -m 0755 -s dnetinfo.sh $(prefix)/sbin/dnetinfo
	install -m 0644 dnroute.8 $(manprefix)/man/man8
	install -m 0644 dnetinfo.8 $(manprefix)/man/man8

clean:
	rm -f $(DNROUTE) *~ *.o netlink/*.o netlink/*.a
