include ../Makefile.common

DNROUTE=dnroute

CC=gcc -O2 -Wall -Wstrict-prototypes -Inetlink/include

all: $(DNROUTE)

$(DNROUTE): get_neigh.c send_route.c routing_msg.c csum.c netlink/libnetlink.a
	$(CC) $(CFLAGS) -o $@ $^ -Lnetlink -ldnet -lnetlink

netlink/libnetlink.a:
	$(MAKE) -C netlink

clean:
	rm -f $(DNROUTE) *~ *.o netlink/*.o netlink/*.a
