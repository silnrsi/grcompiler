AUTOMAKE_OPTIONS = 1.6

EXTRA_DIST = fonts

noinst_PROGRAMS = GrcRegressionTest

AM_CPPFLAGS = -I$(top_srcdir)/compiler $(GR2_CFLAGS)

GrcRegressionTest_LDFLAGS = $(GR2_LIBS)

GrcRegressionTest_SOURCES = \
        main.h \
        stdafx.h \
        TestCase.h \
        CompareFontTables.cpp \
        FileFont.cpp \
        FontStub.cpp \
        GrcRtFileFont.h \
        GrcRtFileFont.cpp \
        GrcRegressionTest.cpp \
        TtfUtil.cpp \
	TtfUtil.h \
	FileInput.h \
	FontTableCache.h

check-local: GrcRegressionTest
	$(top_builddir)/test/GrcRegressionTest

dist-hook:
	rm -f grcregtest.log

