# Makefile for phone progs

include ../Makefile.common

#
# Look for gtk-config on the path
#
# We need GTK+ version 1.2. I daren't say 1.2 or higher (tho' I will
# allow point releases) because GTK+ changes so much between releases.
#
# Don't make a GTK version for release binaries because it adds too many
# unnecessary dependancies
#
GTK_VERSION=$(shell gtk-config --version 2>/dev/null | cut -d. -f1-2)
ifndef RELEASE
ifeq ($(GTK_VERSION), 1.2)
HAVE_GTK=true
INSTALLDEPS=install_pixmaps
PIXMAPDIR=$(prefix)/X11/pixmaps/phone
endif
endif

LIBPANEL=-lpanel
LIBNCURSES=-lncurses

PROG1=phone
PROG2=phoned

MANPAGES1=phone.1
MANPAGES8=phoned.8

PROG1OBJS=main.o phone_ncurses.o backend.o
PROG2OBJS=phoned.o phone_server.o

all: $(PROG1) $(PROG2)

ifdef HAVE_GTK
PROG1OBJS+=gtkphonesrc.o gtkphonesig.o phone_gtk.o
GTKLIBS=$(shell gtk-config --libs)
CFLAGS+=-DHAVE_GTK $(BINARY_PREFIX)
endif

$(PROG1): $(PROG1OBJS) $(DEPLIBDNET)
	$(CC) $(CFLAGS) -o $@ $(PROG1OBJS) $(LIBPANEL) $(LIBNCURSES) $(LIBDNET) $(GTKLIBS)
  
$(PROG2): $(PROG2OBJS) $(DEPLIBDNET)
	$(CC) $(CFLAGS) -o $@ $(PROG2OBJS) $(LIBDNET)

#
# These will only be built if HAVE_GTK is defined
#
gtkphonesrc.o: gtkphonesrc.c
	$(CC) $(CFLAGS) -c -o $@ $< `gtk-config --cflags`

gtkphonesig.o: gtkphonesig.c
	$(CC) $(CFLAGS) -c -o $@ $< `gtk-config --cflags`

phone_gtk.o: phone_gtk.c
	$(CC) $(CFLAGS) -c -o $@ $< `gtk-config --cflags`

install: $(INSTALLDEPS)
	install -d $(prefix)/bin
	install -d $(prefix)/man/man1
	install -d $(prefix)/man/man8
	install -m 0755 -s $(PROG1) $(prefix)/bin
	install -m 0755 -s $(PROG2) $(prefix)/sbin
	install -m 0644 $(MANPAGES1) $(prefix)/man/man1
	install -m 0644 $(MANPAGES8) $(prefix)/man/man8

install_pixmaps:
	install -d $(PIXMAPDIR)
	install -m 0644 pixmaps/*.xpm $(PIXMAPDIR)


dep depend:	
	makedepend *.cc *.c 2>/dev/null

clean:
	makedepend
	rm -f $(PROG1) $(PROG2) *.o *.bak


# DO NOT DELETE THIS LINE -- make  depend  depends  on it.
