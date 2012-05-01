VERSION=$(shell ./get_version.sh)

all:

srctar:
	if [ -r flac123/Makefile ];then (cd flac123;make distclean); fi
	mkdir /tmp/flac123-${VERSION}
	(cd flac123;tar cf - . ) | (cd /tmp/flac123-${VERSION}; tar xf -)
	(cd /tmp;tar -c -z --exclude .svn -f flac123-${VERSION}.tar.gz flac123-${VERSION})
	mv /tmp/flac123-${VERSION}.tar.gz .
	rm -rf /tmp/flac123-${VERSION} 
