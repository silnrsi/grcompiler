AC_PREREQ(2.54)

AC_INIT(grcompiler, 5.2.1, silgraphite-devel@lists.sourceforge.net)
AC_CONFIG_SRCDIR(compiler/GrpParser.g)
AC_CONFIG_AUX_DIR(admin)

# setup automake
AM_INIT_AUTOMAKE([foreign])
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
#AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB
PKG_PROG_PKG_CONFIG

AC_PATH_PROG(LSB_RELEASE, lsb_release, no)

# docbook2man utility
AC_ARG_VAR(DOCBOOK2MAN, [Program used to process XML man pages])
AC_CHECK_PROGS(DOCBOOK2MAN, docbook2x-man, no)
if test "$DOCBOOK2MAN" = no; then
    AC_MSG_FAILURE([Could not find a program to process XML man pages])
fi

# Check packages needed
PKG_CHECK_MODULES([ICU], [icu-i18n > 3.6])
LIBS="$LIBS $ICU_LIBS"

# Set default language for tests

AC_LANG(C++)
# Checks for libraries.

# Checks for header files.
# need to check for icu .h files as in unicode/uchar.h
AC_CHECK_HEADER(unicode/uchar.h,,)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

# Checks for library functions.

AC_CHECK_DECLS([program_invocation_short_name],,,[[#include <errno.h>]])

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

# Find distrib codename

AC_MSG_CHECKING(if already have REL_CODENAME)
if test -z "${REL_CODENAME}"; then
	AC_MSG_RESULT(no)
	AC_MSG_CHECKING(if have lsb_release)
	if test "${LSB_RELEASE}" = "no"; then
		AC_MSG_RESULT(no)
		REL_CODENAME="unstable"
	else
		AC_MSG_RESULT(yes)
		AC_MSG_CHECKING(if LSB_RELEASE is empty)
		if test -z ${LSB_RELEASE}; then
			AC_MSG_RESULT(yes)
			REL_CODENAME="unstable"
		else
			AC_MSG_RESULT(no)
			CODENAME=`${LSB_RELEASE} -c | sed -e 's/^\S*\s*//'`
			AC_MSG_CHECKING(if release is sid)
			if test ${CODENAME} = 'sid'; then
				AC_MSG_RESULT(yes)
				REL_CODENAME="unstable"
			else
				AC_MSG_RESULT(no, ${CODENAME})
				REL_CODENAME=${CODENAME}
			fi
		fi
	fi
else
	AC_MSG_RESULT(yes)
fi

AC_SUBST(REL_CODENAME)


#CXXFLAGS="$CXXFLAGS -DGR_NAMESPACE=1"

AC_CONFIG_FILES(Makefile \
	doc/Makefile \
	preprocessor/Makefile \
	compiler/Makefile \
	compiler/Generic/Makefile \
	compiler/Grammar/Makefile \
	compiler/LZ4/Makefile \
    test/Makefile)

AC_CONFIG_SUBDIRS([test/GrcRegressionTest])

AC_OUTPUT
