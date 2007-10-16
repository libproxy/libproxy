#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([libproxy],[0.1],[nathaniel@natemccallum.com])
AC_CONFIG_SRCDIR([src/lib/proxy_factory.c])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AC_ARG_WITH([mozjs],
            [AS_HELP_STRING([--with-mozjs],
              [build mozilla javascript PAC runner plugin @<:@default=check@:>@])],
            [],
            [PKG_CHECK_EXISTS(firefox-js, with_mozjs=yes, with_mozjs=no)])
if test "$with_mozjs" = yes; then
    PKG_CHECK_MODULES(MOZJS, firefox-js)
	AC_SUBST(MOZJS_CFLAGS)
	AC_SUBST(MOZJS_LIBS)
fi
AM_CONDITIONAL(WITH_MOZJS, [test x$with_mozjs = xyes])

# Checks for header files.
AC_HEADER_STDC
#AC_CHECK_HEADERS([netdb.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
PLUGINDIR=$libdir/$PACKAGE_NAME/$PACKAGE_VERSION/plugins
AC_SUBST(PLUGINDIR)
CFLAGS="-std=c99 $CFLAGS -DPLUGINDIR=\\\"$PLUGINDIR\\\""

# Checks for library functions.
AC_FUNC_MALLOC
#AC_CHECK_FUNCS([gethostbyname gethostname memset socket strdup strstr])

AC_CONFIG_FILES([Makefile src/Makefile src/lib/Makefile src/plugins/Makefile src/bin/Makefile])
AC_OUTPUT
