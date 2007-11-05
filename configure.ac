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

### Checks for libraries for plugins.
# Mozilla Javascript
AC_ARG_WITH([mozjs],
            [AS_HELP_STRING([--with-mozjs],
              [build Mozilla JavaScript PAC runner plugin @<:@automatic@:>@])],
            [],
            [PKG_CHECK_EXISTS(firefox-js, with_mozjs=yes, with_mozjs=no)])
if test x$with_mozjs = xyes; then
	PKG_CHECK_MODULES(MOZJS, firefox-js)
	AC_SUBST(MOZJS_CFLAGS)
	AC_SUBST(MOZJS_LIBS)
fi
AM_CONDITIONAL([WITH_MOZJS], [test x$with_mozjs = xyes])

# GConf
AC_ARG_WITH([gnome],
            [AS_HELP_STRING([--with-gnome],
              [build GConf configuration plugin @<:@automatic@:>@])],
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
AM_CONDITIONAL([WITH_GNOME], [test x$with_gnome = xyes])

# NetworkManager
AC_ARG_WITH([networkmanager],
            [AS_HELP_STRING([--with-networkmanager],
              [build NetworkManager plugin @<:@automatic@:>@])],
            [],
            [PKG_CHECK_EXISTS(NetworkManager, with_networkmanager=yes, with_networkmanager=no)])
if test x$with_networkmanager = xyes; then
	PKG_CHECK_MODULES(NETWORKMANAGER, NetworkManager)
	AC_SUBST(NETWORKMANAGER_CFLAGS)
	AC_SUBST(NETWORKMANAGER_LIBS)
fi
AM_CONDITIONAL([WITH_NETWORKMANAGER], [test x$with_networkmanager = xyes])

### Check for binding requirements
# Python
AC_ARG_WITH([python],
            [AS_HELP_STRING([--with-python],
              [build Python bindings @<:@automatic@:>@])],
            [AM_PATH_PYTHON([2.5], with_python=yes)],
            [AM_PATH_PYTHON([2.5], with_python=yes, with_python=no)])
AM_CONDITIONAL([WITH_PYTHON], [test x$with_python = xyes])

# Java
with_java=no
AM_CONDITIONAL([WITH_JAVA], [test x$with_java = xyes])

# .NET
with_dotnet=no
AM_CONDITIONAL([WITH_DOTNET], [test x$with_dotnet = xyes])

### Checks for header files.
AC_HEADER_STDC
#AC_CHECK_HEADERS([netdb.h stdlib.h string.h sys/socket.h unistd.h])

### Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
PLUGINDIR=$libdir/$PACKAGE_NAME/$PACKAGE_VERSION/plugins
AC_SUBST(PLUGINDIR)
CFLAGS="-std=c99 $CFLAGS -DPLUGINDIR=\\\"$PLUGINDIR\\\" -DSYSCONFDIR=\\\"$SYSCONFDIR\\\" -D_POSIX_C_SOURCE=1"

### Checks for library functions.
AC_FUNC_MALLOC
#AC_CHECK_FUNCS([gethostbyname gethostname memset socket strdup strstr])

AC_CONFIG_FILES([Makefile src/Makefile src/lib/Makefile src/plugins/Makefile 
                 src/bin/Makefile src/bindings/Makefile src/bindings/python/Makefile
                 src/bindings/java/Makefile src/bindings/dotnet/Makefile])
AC_OUTPUT

### Print build summary
echo
echo "------------------------------------------------------"
echo -e "\tPlugins to build..."
echo -e "\t\tenvvar\t\t\tyes"
echo -e "\t\tmozjs\t\t\t$with_mozjs"
echo -e "\t\tgnome\t\t\t$with_gnome"
echo -e "\t\tnetworkmanager\t\t$with_networkmanager"
echo
echo -e "\tBindings to build..."
echo -e "\t\tpython\t\t\t$with_python"
echo -e "\t\tjava\t\t\t$with_java"
echo -e "\t\tdotnet\t\t\t$with_dotnet"
echo "------------------------------------------------------"
echo

