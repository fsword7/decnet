# Top level Makefile for DECnet programs

include Makefile.common

#
# PKGNAME determines the .tar.gz file name and also is the directory name
#
PKGNAME=dnprogs

SUBDIRS=include libdnet libdaemon libdap librms fal dndir dnsubmit dndel \
	dncopy apps dntask mail phone dnetd scripts

all: 	
	@. scripts/check_kernel.sh; if [ $$? = 1 ]; then exit 1; fi
	@for i in $(SUBDIRS); do make -C $$i $@ ; done

# Unconditional build - for those brave souls who "know what they're doing".
please:
	@echo -e \\a I really hope you know what you are doing!
	@sleep 5
	for i in $(SUBDIRS); do make -C $$i all ; done

install:
	@for i in $(SUBDIRS); do make -C $$i $@ ; done

dep depend:	
	@for i in $(SUBDIRS); do make -C $$i $@ ; done

tags:	
	etags */*.cc */*.c


backup:
	rm -f /var/tmp/$(PKGNAME).tgz
	tar -czvf /var/tmp/$(PKGNAME).tgz -Xexcludes-backup . 
	cp /var/tmp/$(PKGNAME).tgz /msdos/dev/bosy/

clean:
	rm -rf debian/tmp
	rm -rf rpmbuild
	@for i in $(SUBDIRS); do make -C $$i $@ ; done

#
# Make the distribution tar file
#
dist:	
	for i in $(SUBDIRS); do cd $$i; makedepend; cd ..; done
	if [ -L ../$(PKGNAME)-$(VERSION) ]; then rm ../$(PKGNAME)-$(VERSION); fi
	if [ ! -d ../$(PKGNAME)-$(VERSION) ]; then cd ..; ln -s $(PKGNAME) $(PKGNAME)-$(VERSION); fi
	cd ..; tar -czvhf /var/tmp/$(PKGNAME)-$(VERSION).tar.gz -X$(PKGNAME)-$(VERSION)/excludes-dist $(PKGNAME)-$(VERSION);
	if [ -L ../$(PKGNAME)-$(VERSION) ]; then rm ../$(PKGNAME)-$(VERSION); fi

#
# Make a snapshot release
#
snap:	DATE = $(shell date +'%Y%m%d')
snap:
	for i in $(SUBDIRS); do cd $$i; makedepend; cd ..; done	
	if [ -L ../$(PKGNAME)-$(DATE) ]; then rm ../$(PKGNAME)-$(DATE); fi
	if [ ! -d ../$(PKGNAME)-$(DATE) ]; then cd ..; ln -s $(PKGNAME) $(PKGNAME)-$(DATE); fi
	cd ..; tar -czvhf /var/tmp/$(PKGNAME)-$(DATE).tar.gz -X$(PKGNAME)-$(DATE)/excludes-dist $(PKGNAME)-$(DATE);
	if [ -L ../$(PKGNAME)-$(DATE) ]; then rm ../$(PKGNAME)-$(DATE); fi

#
# Make RPM package for Red Hat systems.
#
rpm:
	@if [ $$UID != 0 ]; then echo You must be root to make RPMs; exit 1; fi
	rm -rf rpmbuild
	make clean
	make prefix=/usr RELEASE=true -j
	make DESTDIR=`pwd`/rpmbuild install
	make dist
	cp /var/tmp/$(PKGNAME)-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	install -d rpmbuild/etc/rc.d/init.d
	install -d rpmbuild/etc/rc.d/rc3.d
	install -d rpmbuild/usr/doc
	install scripts/decnet.sh rpmbuild/etc/rc.d/init.d/decnet
	ln -sf ../init.d/decnet rpmbuild/etc/rc.d/rc3.d/S09decnet
	ln -sf libdnet.so.2 rpmbuild/usr/lib/libdnet.so.1
	LIBCPPLINK=`ldd $(prefix)/sbin/fal|grep stdc++|cut -d' ' -f3` ; \
	LIBCPPNAME=`ls -l $$LIBCPPLINK|sed 's/.*-> \(\)/\1/g'` ;        \
	install -m 755 $$LIBCPPLINK `pwd`/rpmbuild/usr/lib;             \
	install -m 755 /usr/lib/$$LIBCPPNAME `pwd`/rpmbuild/usr/lib;    \
	sed -e's/%%PACKAGENAME%%/$(PKGNAME)/g'                          \
	    -e's/%%VERSION%%/$(VERSION)/g'                              \
	    -e's/%%MAJOR_VERSION%%/$(MAJOR_VERSION)/g'                  \
	    -e's@%%PREFIX%%@/usr@g'                                     \
	    -e's@%%LIBPREFIX%%@/usr@g'                                  \
	    -e's@%%CONFPREFIX%%@/@g'                                    \
	    -e"s@%%LIBCPPLINK%%@$$LIBCPPLINK@g"                         \
	    -e"s@%%LIBCPPNAME%%@/usr/lib/$$LIBCPPNAME@g"                \
	   < rpm.spec >$(PKGNAME).spec
	cp README NEWS Documentation/*.README /usr/src/redhat/BUILD
	rpm -bb --buildroot `pwd`/rpmbuild -v $(PKGNAME).spec
	rm -f $(PKGNAME).spec

#
# Make Debian package. It's best to do this immediately after 'make rpm'
#
deb:
	@if [ $$UID != 0 ]; then echo You must be root to make DEBs; exit 1; fi
	rm -f Documentation/*~ Documentation/*.bak
	sed -e's/%%PACKAGENAME%%/$(PKGNAME)/g'        \
	    -e's/%%VERSION%%/$(VERSION)/g'            \
	    -e's@%%PREFIX%%@$(prefix)@g'              \
	    -e"s/%%DATE%%/`date --rfc-822`/g"         \
	   <debian-changelog >debian/changelog
	echo >>debian/changelog
	cat NEWS >>debian/changelog
	if [ -L ../$(PKGNAME)-$(VERSION) ]; then rm ../$(PKGNAME)-$(VERSION); fi
	if [ ! -d ../$(PKGNAME)-$(VERSION) ]; then cd ..; ln -s $(PKGNAME) $(PKGNAME)-$(VERSION); fi
	cd ../$(PKGNAME)-$(VERSION); debian/rules binary-arch
	if [ -L ../$(PKGNAME)-$(VERSION) ]; then rm ../$(PKGNAME)-$(VERSION); fi

#
# Copy binaries to the web preload directory.
# Only really works on my Intel machine, hence the check
#
bindist: dist rpm deb
	@if [ `uname -n` != "pandh" ]; then echo "Only do this on Patrick\'s Intel box"; exit 1; fi
	cp /usr/src/redhat/SOURCES/$(PKGNAME)-$(VERSION).tar.gz /home/patrick/public_html/decnet
	cp ../$(PKGNAME)_$(VERSION)-1_i386.deb /home/patrick/public_html/decnet
	cp /usr/src/redhat/RPMS/$(ARCH)/$(PKGNAME)-$(VERSION)-1.i386.rpm /home/patrick/public_html/decnet
	cp /scratch/sparclinux/$(PKGNAME)-$(VERSION)-1.sparc.rpm /home/patrick/public_html/decnet

#
# Dummy rule for sub-directories
#
dummy:


# DO NOT DELETE THIS LINE -- make  depend  depends  on it.