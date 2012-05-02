VERSION=$(shell ./get_version.sh)
PACKAGE=flac123
DEPENDS=libao4,libflac8
DISTROS=lucid natty oneiric precise
#DISTROS=lucid
DVERSION=${VERSION}.${DISTRO}
UDIR=ubuntu/${PACKAGE}-${DVERSION}
DDIR=${UDIR}/debian
PPA=ppa:0k-hans-f8/flac123

all:

srctar:
	if [ -r ${PACKAGE}/Makefile ];then (cd ${PACKAGE};make distclean); fi
	mkdir /tmp/${PACKAGE}-${VERSION}
	(cd ${PACKAGE};tar cf - . ) | (cd /tmp/${PACKAGE}-${VERSION}; tar xf -)
	(cd /tmp;tar -c -z --exclude .svn -f ${PACKAGE}-${VERSION}.tar.gz ${PACKAGE}-${VERSION})
	mv /tmp/${PACKAGE}-${VERSION}.tar.gz .
	rm -rf /tmp/${PACKAGE}-${VERSION} 

clean:
	rm *.tar.gz
	rm -rf ubuntu

ppa: srctar
	for distro in ${DISTROS}; do make ppa1 DISTRO=$$distro; done

ppa1:
	echo Making PPA for ${DISTRO}
	rm -rf ubuntu
	mkdir ubuntu
	cp ${PACKAGE}-${VERSION}.tar.gz ubuntu/${PACKAGE}_${DVERSION}.orig.tar.gz
	(cd ubuntu; tar xzf ${PACKAGE}_${DVERSION}.orig.tar.gz)
	mv ubuntu/${PACKAGE}-${VERSION} ${UDIR}
	tar cf - debian | (cd ${UDIR}; tar xf -)
	cat ${DDIR}/control.in | sed -e 's/%%DEPENDS%%/${DEPENDS}/g' >${DDIR}/control
	rm ${DDIR}/control.in
	cat ${DDIR}/changelog.in | sed -e 's/%%DISTRO%%/${DISTRO}/g' >${DDIR}/changelog
	rm ${DDIR}/changelog.in
	(cd ${UDIR};debuild -S)
	(cd ubuntu;dput ${PPA} *.changes)

