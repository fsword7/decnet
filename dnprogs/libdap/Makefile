include ../Makefile.common

LIBOBJS=connection.o protocol.o vaxcrc.o logging.o
PICOBJS=connection.po protocol.po vaxcrc.po logging.po

LIBNAME=libdap

SHAREDLIB=$(LIBNAME).so.$(VERSION)
STATICLIB=$(LIBNAME).a


all: $(STATICLIB) $(SHAREDLIB)

$(STATICLIB): $(LIBOBJS)
	ar -rv $@ $^

$(SHAREDLIB): $(PICOBJS)
	$(CXX) $(CXXFLAGS) -shared -o $@ -Wl,-soname=$(LIBNAME).so.$(MAJOR_VERSION) $^
	ln -sf $(SHAREDLIB) $(LIBNAME).so.$(MAJOR_VERSION)
	ln -sf $(LIBNAME).so.$(MAJOR_VERSION) $(LIBNAME).so

.cc.o:
	$(CXX) $(CXXFLAGS) $(SYSCONF_PREFIX) -c -o $@ $<

.cc.po:
	$(CXX) $(CXXFLAGS) $(SYSCONF_PREFIX) -fPIC -c -o $@ $<

dep depend:	
	makedepend *.cc *.c 2>/dev/null

clean:
	makedepend
	rm -f *.o *.po *.bak $(STATICLIB) $(SHAREDLIB) $(LIBNAME).so*

install:
	install -m 0644 -s $(SHAREDLIB) $(libprefix)/lib
	install -m 0644 $(STATICLIB) $(libprefix)/lib
	ln -sf $(SHAREDLIB) $(libprefix)/lib/$(LIBNAME).so.$(MAJOR_VERSION)
	ln -sf $(LIBNAME).so.$(MAJOR_VERSION) $(libprefix)/lib/$(LIBNAME).so

.SUFFIXES: .po

# DO NOT DELETE THIS LINE -- make  depend  depends  on it.
