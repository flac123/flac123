VERSION=$(shell ./get_version.sh)
MYDIST=$(shell cat /etc/lsb-release | grep CODENAME | sed -e 's/[^=]*[=]//')
SUBVERSION=7
PACKAGE=flac123
DEPENDS=libao4,libflac8,libpopt0
DEPENDS1=libao2,libflac8,libpopt0
BUILDDEPENDS=libao-dev,libflac-dev,libpopt-dev,automake,autoconf
DISTROS=natty oneiric precise quantal
DISTROS1=maverick lucid
DVERSION=${VERSION}.${DISTRO}.${SUBVERSION}
UDIR=ubuntu/${PACKAGE}-${DVERSION}
DDIR=${UDIR}/debian
PPA=ppa:0k-hans-f8/flac123

all:
	@echo "make srctar - makes a source tar which needs automake/autoconf"
	@echo "make sf - makes a source release for sourceforge.net that has been automaked/autoconfed"
	@echo ""
	@echo "make ppa - makes a PPA for launchpad for Ubuntu's distros:"
	@echo ""
	@echo "       ${DISTROS}"
	@echo ""
	@echo "make ppa_old - makes a PPA for launchpad for Ubuntu's distros:"
	@echo ""
	@echo "       ${DISTROS1}"
	@echo ""

sf: srctar
	#cp ${PACKAGE}-${VERSION}.tar.gz /tmp
	#(cd /tmp;tar xzf ${PACKAGE}-${VERSION}.tar.gz)
	#(cd /tmp/${PACKAGE}-${VERSION};automake;autoconf)
	#(cd /tmp;tar czf ${PACKAGE}-${VERSION}-release.tar.gz ${PACKAGE}-${VERSION})
	#mv /tmp/${PACKAGE}-${VERSION}-release.tar.gz .
	#rm -rf /tmp/${PACKAGE}-${VERSION}
	cp ${PACKAGE}-${VERSION}.tar.gz ${PACKAGE}-${VERSION}-release.tar.gz

srctar: 
	if [ -r ${PACKAGE}/Makefile ];then (cd ${PACKAGE};make distclean); fi
	mkdir /tmp/${PACKAGE}-${VERSION}
	(cd ${PACKAGE};tar cf - . ) | (cd /tmp/${PACKAGE}-${VERSION}; tar xf -)
	(cd /tmp/${PACKAGE}-${VERSION};automake;autoconf)
	(cd /tmp;tar -c -z --exclude .svn -f ${PACKAGE}-${VERSION}.tar.gz ${PACKAGE}-${VERSION})
	mv /tmp/${PACKAGE}-${VERSION}.tar.gz .
	rm -rf /tmp/${PACKAGE}-${VERSION} 

clean:
	rm *.tar.gz
	rm -rf ubuntu

test:
	make build DISTRO=${MYDIST} DEPEND=${DEPENDS}
	(cd ubuntu;sudo pbuilder build *.dsc)

ppa: 
	for distro in ${DISTROS}; do make ppa1 DISTRO=$$distro DEPEND=${DEPENDS}; done

ppa_old: 
	for distro in ${DISTROS1}; do make ppa1 DISTRO=$$distro DEPEND=${DEPENDS1}; done

ppa1: build
	(cd ubuntu;dput ${PPA} *.changes)

build: srctar
	echo Making PPA for ${DISTRO}
	rm -rf ubuntu
	mkdir ubuntu
	cp ${PACKAGE}-${VERSION}.tar.gz ubuntu/${PACKAGE}_${DVERSION}.orig.tar.gz
	(cd ubuntu; tar xzf ${PACKAGE}_${DVERSION}.orig.tar.gz)
	mv ubuntu/${PACKAGE}-${VERSION} ${UDIR}
	tar cf - debian | (cd ${UDIR}; tar xf -)
	cat ${DDIR}/control.in | sed -e 's/%%DEPENDS%%/${DEPEND}/g' | sed -e 's/%%BUILDDEPENDS%%/${BUILDDEPENDS}/g' >${DDIR}/control
	rm ${DDIR}/control.in
	cat ${DDIR}/changelog.in | sed -e 's/%%DISTRO%%/${DISTRO}/g' | sed -e 's/%%SUBVERSION%%/${SUBVERSION}/g' >${DDIR}/changelog
	rm ${DDIR}/changelog.in
	(cd ${UDIR};debuild -S)

web:
	(cd website;scp *.html *.css *.jpg *.png  hoesterholt@web.sourceforge.net:/home/project-web/flac-tools/htdocs;exit 0)



