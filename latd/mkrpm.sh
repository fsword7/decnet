#!/bin/sh
#
# Make an RPM
#

# Get the package and version from the configure script
eval `grep ^PACKAGE= configure`
eval `grep ^VERSION= configure`
ARCH=`uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ -e s/arm.*/arm/ -e s/sa110/arm/`
export PACKAGE VERSION ARCH

echo "%_topdir `pwd`" > .rpmmacros
echo "`rpm --showrc|grep \^macrofiles`:`pwd`/.rpmmacros" >.rpmrc
rm -rf rpmbuild BUILD RPMS config.cache
./configure --prefix=/usr --sysconfdir=/
make clean
make
make DESTDIR=`pwd`/rpmbuild install
install -d rpmbuild/usr/doc
install -d rpmbuild/etc
install -d rpmbuild/usr/doc/${PACKAGE}-${VERSION}
install latd.conf rpmbuild/etc/latd.conf
install -Dm 0700 startlat.sh rpmbuild/etc/rc.d/init.d/lat
install -d rpmbuild/etc/rc.d/rc3.d
ln -sf ../init.d/lat rpmbuild/etc/rc.d/rc3.d/S79lat
ln -sf ../init.d/lat rpmbuild/etc/rc.d/rc3.d/K79lat
sed -e"s/%%PACKAGENAME%%/${PACKAGE}/g"                          \
    -e"s/%%VERSION%%/${VERSION}/g"                              \
    -e"s@%%PREFIX%%@/usr@g"                                     \
   < rpm.spec >${PACKAGE}.spec
mkdir BUILD
install README NEWS lat.html BUILD
mkdir -p RPMS
rpmbuild -bb --target ${ARCH} --buildroot `pwd`/rpmbuild --rcfile .rpmrc -v \
    ${PACKAGE}.spec
rm -f ${PACKAGE}.spec .rpmrc .rpmmacros
