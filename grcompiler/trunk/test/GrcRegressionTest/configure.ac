AC_PREREQ(2.54)

AC_INIT(GrcRegressionTest, 1.0, silgraphite-devel@lists.sourceforge.net)
AC_CONFIG_SRCDIR(Makefile.am)
AC_CONFIG_AUX_DIR(config)

# setup automake
AM_INIT_AUTOMAKE(foreign)
AM_MAINTAINER_MODE

AC_PROG_CXX

# Check programs we need
# Check packages needed
PKG_CHECK_MODULES(GR2, silgraphite >= 2.2.0)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_CHECK_SIZEOF(wchar_t)
AC_SUBST(SIZEOF_WCHAR_T)

AC_CONFIG_FILES(Makefile)
AC_OUTPUT

