include ../Makefile.common

DNROUTE=dnroute

CC=gcc -O2 -Wall -Wstrict-prototypes -Inetlink/include

all: $(DNROUTE)

$(DNROUTE): get_neigh.c send_route.c routing_msg.c csum.c netlink/libnetlink.a
	$(CC) $(CFLAGS) -o $@ $^ -Lnetlink -ldnet -lnetlink

netlink/libnetlink.a:
	$(MAKE) -C netlink

install:
	install -d $(prefix)/sbin
	install -d $(prefix)/man/man8
	install -m 0755 -s dnroute $(prefix)/sbin
	install -m 0644 dnroute.8 $(prefix)/man/man8

clean:
	rm -f $(DNROUTE) *~ *.o netlink/*.o netlink/*.a
