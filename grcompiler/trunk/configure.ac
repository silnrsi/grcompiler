AC_PREREQ(2.54)

AC_INIT(grcompiler, 1.9, silgraphite-devel@lists.sourceforge.net)
AC_CONFIG_SRCDIR(compiler/GrpParser.g)
AC_CONFIG_AUX_DIR(admin)

# setup automake
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

# System checks.
AC_CANONICAL_HOST
AM_ICONV

# Optional features
AC_ARG_ENABLE(debug,
    AC_HELP_STRING([--enable-debug], [build with debugging. (default=no)]),
    [enable_debug=yes], [enable_debug=no])
AC_ARG_ENABLE(final,
    AC_HELP_STRING([--enable-final], [build with optimizations and no debugging. (default=no)]),
    [enable_final=yes], [enable_final=no])
AC_ARG_ENABLE(profile,
    AC_HELP_STRING([--enable-profile], [allow profiling (default=no)]),
    [enable_profile=yes], [enable_profile=no])
AC_ARG_ENABLE(profilefn,
    AC_HELP_STRING([--enable-profilefn], [allow functioncheck profiling (default=no)]),
    [enable_profilefn=yes],[enable_profilefn=no])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Checks for libraries.

AC_CHECK_LIB(sicuuc, u_charType_3_6)

# Checks for header files.
# need to check for icu .h files as in unicode/uchar.h
AC_CHECK_HEADER(unicode/uchar.h,,)

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.


# Setup the build compiler flags and linker flags.
# Enable debug
if test "$enable_debug" != no; then
    CFLAGS="$CFLAGS -O0"
    CXXFLAGS="$CXXFLAGS -O0"
    test "$ac_cv_prog_cc_g" = yes     && CFLAGS="$CFLAGS -g"
    test "$ac_cv_prog_cxx_g" = yes    && CXXFLAGS="$CXXFLAGS -g"
    CFLAGS="$CFLAGS -Wno-unknown-pragmas -Wparentheses -Werror "
    CXXFLAGS="$CXXFLAGS -Wno-unknown-pragmas -Wparentheses -Werror "
else
    CFLAGS="$CFLAGS -DNDEBUG"
    CXXFLAGS="$CXXFLAGS -DNDEBUG"
fi

# Enable final
if test "$enable_final" != no; then
    CFLAGS="-O3 -DNDEBUG"
    CXXFLAGS="-O3 -DNDEBUG"
fi

# Enable profile
if test "$enable_profile" != no; then
    CFLAGS="$CFLAGS -pg "
    CXXFLAGS="$CXXFLAGS -pg "
fi
if test "$enable_profilefn" != no; then
    CFLAGS="$CFLAGS -g -finstrument-functions "
    CXXFLAGS="$CXXFLAGS -g -finstrument-functions "
    LIBS="$LIBS -g -finstrument-functions -lfnccheck "
fi

#CXXFLAGS="$CXXFLAGS -DGR_NAMESPACE=1"

AC_CONFIG_FILES(Makefile \
	doc/Makefile \
	preprocessor/Makefile \
	compiler/Makefile \
	compiler/Generic/Makefile \
	compiler/Grammar/Makefile \
    test/Makefile)

AC_CONFIG_SUBDIRS([test/GrcRegressionTest])

AC_OUTPUT
