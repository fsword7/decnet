include ../Makefile.common

IPROUTE2=/usr/local/src/iproute2

DNROUTE=dnroute

CC=gcc -g -Wall -Wstrict-prototypes -I$(IPROUTE2)/include

all: $(DNROUTE)

$(DNROUTE): get_neigh.c send_route.c routing_msg.c csum.c
	$(CC) $(CFLAGS) -o $@ $^ -L$(IPROUTE2)/lib -lnetlink -lutil -ldnet

clean:
	rm -f $(DNROUTE) *~ *.o
