AC_PREREQ(2.61)
AC_INIT([libproxy],[0.2],[nathaniel@natemccallum.com])
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
# Environmental Variable
AC_ARG_WITH([envvar],
            [AS_HELP_STRING([--with-envvar],
              [build environmental variable configuration plugin @<:@default=yes@:>@])],
            [], [with_envvar=yes])
AM_CONDITIONAL([WITH_ENVVAR], [test x$with_envvar = xyes])

# File
AC_ARG_WITH([file],
            [AS_HELP_STRING([--with-file],
              [build file-based configuration plugin @<:@default=yes@:>@])],
            [], [with_file=yes])
AM_CONDITIONAL([WITH_FILE], [test x$with_envvar = xyes])
            
# GNOME
AC_ARG_WITH([gnome],
            [AS_HELP_STRING([--with-gnome],
              [build GNOME configuration plugin @<:@automatic@:>@])],
            [],
            [PKG_CHECK_EXISTS(gconf-2.0, with_gnome=yes, with_gnome=no)])
if test x$with_gnome = xyes; then
	PKG_CHECK_MODULES(GCONF, gconf-2.0)
	AC_SUBST(GCONF_CFLAGS)
	AC_SUBST(GCONF_LIBS)
fi
AM_CONDITIONAL([WITH_GNOME], [test x$with_gnome = xyes])

# KDE
AC_ARG_WITH([kde],
            [AS_HELP_STRING([--with-kde],
              [build KDE configuration plugin @<:@default=yes@:>@])],
            [], [with_kde=yes])
AM_CONDITIONAL([WITH_KDE], [test x$with_kde = xyes])

# Mozilla Javascript
AC_ARG_WITH([mozjs],
            [AS_HELP_STRING([--with-mozjs],
              [build Mozilla JavaScript PAC runner plugin @<:@automatic@:>@])],
            [],
            [PKG_CHECK_EXISTS(firefox-js, with_mozjs=yes, 
             PKG_CHECK_EXISTS(xulrunner-js, with_mozjs=yes, with_mozjs=no))])
if test x$with_mozjs = xyes; then
	PKG_CHECK_MODULES(MOZJS, firefox-js,, [PKG_CHECK_MODULES(MOZJS, xulrunner-js)])
	AC_SUBST(MOZJS_CFLAGS)
	AC_SUBST(MOZJS_LIBS)
fi
AM_CONDITIONAL([WITH_MOZJS], [test x$with_mozjs = xyes])

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
CFLAGS="-std=c99 $CFLAGS -DPLUGINDIR=\\\"$PLUGINDIR\\\" -DSYSCONFDIR=\\\"$sysconfdir\\\" -D_POSIX_C_SOURCE=1"

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
echo -e "\t\tenvvar          : $with_envvar"
echo -e "\t\tfile            : $with_file"
echo -e "\t\tgnome           : $with_gnome"
echo -e "\t\tkde             : $with_kde"
echo -e "\t\tmozjs           : $with_mozjs"
echo -e "\t\tnetworkmanager  : $with_networkmanager"
echo
echo -e "\tBindings to build..."
echo -e "\t\tpython          : $with_python"
echo -e "\t\tjava            : $with_java"
echo -e "\t\tdotnet          : $with_dotnet"
echo "------------------------------------------------------"
echo

