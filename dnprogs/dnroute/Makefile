IPROUTE2=/usr/local/src/iproute2

DNROUTE=dnroute

CC=gcc -g -Wall -fstrict-prototypes -I$(IPROUTE2)/include

all: $(DNROUTE)

$(DNROUTE): dnroute.c dnrtlink.c send_route.c csum.c
	$(CC) $(CFLAGS) -o $@ $^ -L$(IPROUTE2)/lib -lnetlink -lutil -ldnet

clean:
	rm -f $(DNROUTE) *~ *.o
