# Top level Makefile for DECnet programs

include Makefile.common

#
# PKGNAME determines the .tar.gz file name and also is the directory name
#
PKGNAME=dnprogs
DATE="$(shell date +'%Y%m%d')"

SUBDIRS=include libdnet libdaemon libdap librms fal dndir dnsubmit dndel \
	dncopy apps dntask dnlogin mail phone dnetd dnroute scripts

all: 	
	@. scripts/check_kernel.sh; if [ $$? = 1 ]; then exit 1; fi
	@set -e; for i in $(SUBDIRS); do make -C $$i $@ ; done

install:
	@set -e; for i in $(SUBDIRS); do make -C $$i $@ ; done

dep depend:	
	@set -e; for i in $(SUBDIRS); do make -C $$i $@ ; done

tags:	
	etags */*.cc */*.c


clean:
	rm -rf debian/tmp
	rm -f debian/files
	rm -f debian/substvars
	rm -f debian/*~
	rm -rf rpmbuild
	rm -rf RPMS
	rm -f .rpm*
	rm -f build
	rm -f core
	@for i in $(SUBDIRS); do make -C $$i $@ ; done

debclean:
	debian/rules clean
#
# Make the distribution tar file
#
dist:	
	for i in $(SUBDIRS); do cd $$i; rm -f .depend; cd ..; done
	if [ -L ../$(PKGNAME)-$(VERSION) ]; then rm ../$(PKGNAME)-$(VERSION); fi
	if [ ! -d ../$(PKGNAME)-$(VERSION) ]; then cd ..; ln -s $(PKGNAME) $(PKGNAME)-$(VERSION); fi
	cd ..; tar -czvhf /var/tmp/$(PKGNAME)-$(VERSION).tar.gz -X$(PKGNAME)-$(VERSION)/excludes-dist $(PKGNAME)-$(VERSION)/;
	if [ -L ../$(PKGNAME)-$(VERSION) ]; then rm ../$(PKGNAME)-$(VERSION); fi

#
# Make a snapshot release
#
snap:	
	for i in $(SUBDIRS); do cd $$i; rm -f .depend; cd ..; done	
	if [ -L ../$(PKGNAME)-$(DATE) ]; then rm ../$(PKGNAME)-$(DATE); fi
	if [ ! -d ../$(PKGNAME)-$(DATE) ]; then cd ..; ln -s $(PKGNAME) $(PKGNAME)-$(DATE); fi
	cd ..; tar -czvhf /var/tmp/$(PKGNAME)-$(DATE).tar.gz -X$(PKGNAME)-$(DATE)/excludes-dist $(PKGNAME)-$(DATE);
	if [ -L ../$(PKGNAME)-$(DATE) ]; then rm ../$(PKGNAME)-$(DATE); fi

#
# Make RPM package for Red Hat systems.
#
rpm:
	rm -rf rpmbuild BUILD RPMS
	make clean
	echo "%_topdir `pwd`" > .rpmmacros
	echo "`rpm --showrc|grep \^macrofiles`:`pwd`/.rpmmacros" >.rpmrc
	make prefix=/usr RELEASE=true -j
	make DESTDIR=`pwd`/rpmbuild RELEASE=true install
	ln -sf libdnet.so.2 rpmbuild/usr/lib/libdnet.so.1
	make dist
	install -d rpmbuild/etc/rc.d/init.d
	install -d rpmbuild/usr/doc
	sed -e's@/usr/local@/usr@g' < scripts/decnet.sh >rpmbuild/etc/rc.d/init.d/decnet
	sed -e's/%%PACKAGENAME%%/$(PKGNAME)/g'                          \
	    -e's/%%VERSION%%/$(VERSION)/g'                              \
	    -e's/%%MAJOR_VERSION%%/$(MAJOR_VERSION)/g'                  \
	    -e's@%%PREFIX%%@/usr@g'                                     \
	    -e's@%%LIBPREFIX%%@/usr@g'                                  \
	    -e's@%%CONFPREFIX%%@/@g'                                    \
	   < rpm.spec >$(PKGNAME).spec
	mkdir -p BUILD RPMS/$(ARCH)
	cp README NEWS Documentation/*.README BUILD
	rpmbuild -bb --target $(ARCH) --buildroot `pwd`/rpmbuild -v --rcfile .rpmrc $(PKGNAME).spec
	rm -f $(PKGNAME).spec .rpmrc .rpmmacros

#
# Make Debian package.
#
deb:
	rm -f Documentation/*~ Documentation/*.bak
	rm -f dnlogin/*.o dnlogin/dnlogin
	dpkg-buildpackage -rfakeroot

#
# Dummy rule for sub-directories
#
dummy:


# DO NOT DELETE THIS LINE -- make  depend  depends  on it.
