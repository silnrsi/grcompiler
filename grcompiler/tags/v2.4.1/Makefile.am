SUBDIRS = doc preprocessor compiler test

EXTRA_DIST = license makefile.mak

dist-hook:
	find $(distdir) -name ".svn" | xargs rm -rf

VC_VERSION = 2.4
D_VERSION = `svnversion ${top_srcdir}|sed -e "s/[0-9]*\://g"`
DEB_VERSION = $(VC_VERSION)+0$(D_VERSION)svn
DEBPACKAGE = grcompiler-$(DEB_VERSION)
RELPACKAGE = grcompiler-${VC_VERSION}
ORIGTAR = grcompiler_$(DEB_VERSION).orig.tar.gz

DISTRIB_CODENAME ?= ${REL_CODENAME}

showcodename:
	echo "Codename: ${DISTRIB_CODENAME}"

deb-base: dist
	rm -fr ${DEBPACKAGE}
	mkdir -p ${DEBPACKAGE}
	cp ${PACKAGE}-${VERSION}.tar.gz $(DEBPACKAGE)
	tar cfz $(ORIGTAR) $(DEBPACKAGE)
	cp -a ${top_srcdir}/debian $(DEBPACKAGE)
	cd ${DEBPACKAGE} && find . -name .svn | xargs rm -rf

deb: deb-base
	dch -D ${DISTRIB_CODENAME} -v ${DEB_VERSION}-1 -m -c${DEBPACKAGE}/debian/changelog "This is build direct from svn"
	cd ${DEBPACKAGE} && pdebuild

deb-binary-only: deb-base
	dch -D ${DISTRIB_CODENAME} -v ${DEB_VERSION}-1 -m -c${DEBPACKAGE}/debian/changelog "This is build direct from svn"
	cd ${DEBPACKAGE} && pdebuild --debbuildopts -i --debbuildopts -b

deb-release: dist
	rm -fr ${RELPACKAGE}
	mkdir -p ${RELPACKAGE}
	cp ${PACKAGE}-${VERSION}.tar.gz $(RELPACKAGE)
	tar cfz $(ORIGTAR) $(RELPACKAGE)
	cp -a ${top_srcdir}/debian $(RELPACKAGE)
	cd ${RELPACKAGE} && find . -name .svn | xargs rm -rf
	dch -D ${DISTRIB_CODENAME} -v ${VC_VERSION}-1 -c${RELPACKAGE}/debian/changelog
	cd ${RELPACKAGE} && pdebuild

deb-local: deb-base
	dch -D ${DISTRIB_CODENAME} -v ${DEB_VERSION}-1 -m -c${DEBPACKAGE}/debian/changelog "This is a debuild direct from svn"
	cd ${DEBPACKAGE} && debuild -us -uc

