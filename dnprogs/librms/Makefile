include ../Makefile.common

LIBOBJS=open.o close.o readwrite.o getreply.o parse.o
PICOBJS=open.po close.po readwrite.po getreply.po parse.po
EXAMPLE1OBJS=example.o
EXAMPLE2OBJS=t_example.o

LIBNAME=librms

EXAMPLE1=example
EXAMPLE2=t_example
EXAMPLES=$(EXAMPLE1) $(EXAMPLE2)

SHAREDLIB=$(LIBNAME).so.$(VERSION)
STATICLIB=$(LIBNAME).a
INCLUDEFILES=rms.h fabdef.h rabdef.h

CFLAGS+=-I. -I../include -I../libdap -fdollars-in-identifiers

all: $(STATICLIB) $(SHAREDLIB) $(EXAMPLES)

$(STATICLIB): $(LIBOBJS)
	ar -rv $@ $^

$(SHAREDLIB): $(PICOBJS)
	$(CXX) $(CXXFLAGS) -shared -o $@ -Wl,-soname=$(LIBNAME).so.$(MAJOR_VERSION) $^ 
	ln -sf $(SHAREDLIB) $(LIBNAME).so.$(MAJOR_VERSION)
	ln -sf $(LIBNAME).so.$(MAJOR_VERSION) $(LIBNAME).so

$(EXAMPLE1): $(EXAMPLE1OBJS) $(SHAREDLIB)
	$(CC) $(CXXFLAGS) -o $@ $< -L. -lrms $(LIBS)

$(EXAMPLE2): $(EXAMPLE2OBJS) $(SHAREDLIB)
	$(CC) $(CXXFLAGS) -o $@ $< -L. -lrms $(LIBS)

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.cc.po:
	$(CXX) $(CXXFLAGS) -fPIC -c -o $@ $<

dep depend:	
	$(CC) $(CFLAGS) -MM *.cc *.c >.depend 2>/dev/null

clean:
	rm -f *.o *.po *.bak .depend $(STATICLIB) $(SHAREDLIB) $(LIBNAME).so* $(EXAMPLES)

install:
	install -d $(prefix)/lib
	install -m 0644 -s $(SHAREDLIB) $(libprefix)/lib
	install -m 0644 $(STATICLIB) $(libprefix)/lib
	install -d $(prefix)/include
	install -m 0644 $(INCLUDEFILES) $(libprefix)/include
	ln -sf $(SHAREDLIB) $(libprefix)/lib/$(LIBNAME).so.$(MAJOR_VERSION)
	ln -sf $(LIBNAME).so.$(MAJOR_VERSION) $(libprefix)/lib/$(LIBNAME).so

.SUFFIXES: .po

ifeq (.depend,$(wildcard .depend))
include .depend
endif

