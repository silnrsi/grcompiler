AUTOMAKE_OPTIONS = 1.6

# EXTRA_DIST = fonts regtest

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
	FontTableCache.h \
	TtfTypes.h \
    fonts \
    regtest

check-local: GrcRegressionTest
	./regtest -g ../../compiler/grcompiler -p ../../preprocessor/gdlpp -r ./GrcRegressionTest fonts

dist-hook:
	rm -f grcregtest.log

