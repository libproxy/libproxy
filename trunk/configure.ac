AC_PREREQ(2.61)
AC_INIT([libproxy],[0.1],[nathaniel@natemccallum.com])
AC_CONFIG_SRCDIR([src/lib/proxy_factory.c])
AM_INIT_AUTOMAKE

### Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET

### Checks for libraries.
# Mozilla Javascript
AC_ARG_WITH([mozjs],
            [AS_HELP_STRING([--with-mozjs],
              [build mozilla javascript PAC runner plugin @<:@default=auto@:>@])],
            [],
            [PKG_CHECK_EXISTS(firefox-js, with_mozjs=yes, with_mozjs=no)])
if test x$with_mozjs = xyes; then
	PKG_CHECK_MODULES(MOZJS, firefox-js)
	AC_SUBST(MOZJS_CFLAGS)
	AC_SUBST(MOZJS_LIBS)
fi
AM_CONDITIONAL(WITH_MOZJS, [test x$with_mozjs = xyes])

# GConf
AC_ARG_WITH([gnome],
            [AS_HELP_STRING([--with-gnome],
              [build GConf configuration plugin @<:@default=auto@:>@])],
            [],
            [PKG_CHECK_EXISTS(gconf-2.0, have_gconf=yes, with_gnome=no) ;
             PKG_CHECK_EXISTS(gdk-2.0, have_gdk=yes, with_gnome=no)])
test x$have_gconf = xyes && test x$have_gdk = xyes && with_gnome=yes
if test x$with_gnome = xyes; then
	PKG_CHECK_MODULES(GCONF, gconf-2.0)
	PKG_CHECK_MODULES(GDK, gdk-2.0)
	AC_SUBST(GCONF_CFLAGS)
	AC_SUBST(GCONF_LIBS)
	AC_SUBST(GDK_CFLAGS)
	AC_SUBST(GDK_LIBS)
fi
AM_CONDITIONAL(WITH_GNOME, [test x$with_gnome = xyes])

### Checks for header files.
AC_HEADER_STDC
#AC_CHECK_HEADERS([netdb.h stdlib.h string.h sys/socket.h unistd.h])

### Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
PLUGINDIR=$libdir/$PACKAGE_NAME/$PACKAGE_VERSION/plugins
AC_SUBST(PLUGINDIR)
CFLAGS="-std=c99 $CFLAGS -DPLUGINDIR=\\\"$PLUGINDIR\\\""

### Checks for library functions.
AC_FUNC_MALLOC
#AC_CHECK_FUNCS([gethostbyname gethostname memset socket strdup strstr])

AC_CONFIG_FILES([Makefile src/Makefile src/lib/Makefile src/plugins/Makefile src/bin/Makefile])
AC_OUTPUT

### Print build summary
echo
echo "------------------------------------------------------"
echo -e "\tBuilding..."
echo -e "\t\tenvvar\tyes"
echo -e "\t\tmozjs\t$with_mozjs"
echo -e "\t\tgnome\t$with_gnome"
echo "------------------------------------------------------"

