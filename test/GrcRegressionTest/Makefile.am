AUTOMAKE_OPTIONS = 1.6

# EXTRA_DIST = fonts regtest

noinst_PROGRAMS = GrcRegressionTest

AM_CPPFLAGS = -I$(top_srcdir)/../../compiler

GrcRegressionTest_LDFLAGS = $(GR2_LIBS)

GrcRegressionTest_SOURCES = \
        main.h \
        stdafx.h \
        TestCase.h \
        CompareFontTables.cpp \
        FileInput.cpp \
        GrcRtFileFont.h \
        GrcRtFileFont.cpp \
        GrcRegressionTest.cpp \
        FileInput.h \
        FontTableCache.h \
        $(top_srcdir)/../../compiler/TtfUtil.cpp \
        $(top_srcdir)/../../compiler/TtfUtil.h \
        $(top_srcdir)/../../compiler/TtfTypes.h \
        fonts \
        regtest

# FontStubb.cpp

check-local: GrcRegressionTest
	$(top_srcdir)/regtest -k -v -g ../../compiler/grcompiler -p ../../preprocessor/gdlpp -r ./GrcRegressionTest -d . $(top_srcdir)/fonts

dist-hook:
	rm -f regtest.log gdlerr.txt grcregtest.log
	find $(distdir) -name "dbg_*" | xargs rm -rf
	rm -f *Test*.ttf

CLEANFILES=regtest.log gdlerr.txt grcregtest.log *Test*.ttf
