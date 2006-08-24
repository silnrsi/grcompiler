SUBDIRS = doc preprocessor compiler

EXTRA_DIST = license makefile.mak

#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_DATA = silgraphite.pc

#$(pkgconfig_DATA): config.status

dist-hook:
	find $(distdir) -name ".svn" | xargs rm -rf
