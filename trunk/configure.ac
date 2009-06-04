AC_PREREQ(2.61)
AC_INIT([libproxy],[0.3.0],[nathaniel@natemccallum.com])
AC_CONFIG_SRCDIR([src/lib/proxy.c])
AM_INIT_AUTOMAKE

### Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_CC_C_O

### Check for pthread
AC_CHECK_HEADERS(pthread.h)
AC_CHECK_FUNCS(pthread_mutex_init)

### Check for socket functions (opensolaris requires -lsocket for example)
AC_SEARCH_LIBS([socket],[socket inet])
AC_SEARCH_LIBS([dlopen],[dl])

### Checks for libraries for modules.
PKG_CHECK_MODULES(x11, x11, have_x11=yes, have_x11=no)
PKG_CHECK_MODULES(xmu, xmu, have_xmu=yes, have_xmu=no)
PKG_CHECK_MODULES(gconf, gconf-2.0, have_gconf=yes, have_gconf=no)
PKG_CHECK_MODULES(webkit, webkit-1.0, have_webkit=yes, have_webkit=no)
PKG_CHECK_MODULES(mozjs, xulrunner-js, have_mozjs=yes, 
		[PKG_CHECK_MODULES(mozjs, firefox-js, have_mozjs=yes,
		[PKG_CHECK_MODULES(mozjs, mozilla-js, have_mozjs=yes, 
		[PKG_CHECK_MODULES(mozjs, seamonkey-js, have_mozjs=yes,have_mozjs=no)])])])
PKG_CHECK_MODULES(NetworkManager, NetworkManager dbus-1, 
					have_networkmanager=yes, have_networkmanager=no)

# Direct
AC_ARG_WITH([direct],
            [AS_HELP_STRING([--with-direct],
            [build direct configuration module @<:@default=yes@:>@])],
            [], [with_direct=yes])
AM_CONDITIONAL([WITH_DIRECT], [test x$with_direct = xyes])

# Environmental Variable
AC_ARG_WITH([envvar],
            [AS_HELP_STRING([--with-envvar],
            [build environmental variable configuration module @<:@default=yes@:>@])],
            [], [with_envvar=yes])
AM_CONDITIONAL([WITH_ENVVAR], [test x$with_envvar = xyes])

# File
AC_ARG_WITH([file],
            [AS_HELP_STRING([--with-file],
            [build file-based configuration module @<:@default=yes@:>@])],
            [], [with_file=yes])
AM_CONDITIONAL([WITH_FILE], [test x$with_envvar = xyes])
            
# GNOME
AC_ARG_WITH([gnome],
            [AS_HELP_STRING([--with-gnome],
            [build GNOME configuration module @<:@automatic@:>@])],
            [],
            [test x$have_gconf = xyes && 
             test x$have_x11   = xyes &&
             test x$have_xmu   = xyes &&
             with_gnome=yes])
if test x$with_gnome = xyes; then
	if test x$have_gconf = xyes && \
	   test x$have_x11 = xyes && \
	   test x$have_xmu = xyes; then
		GNOME_CFLAGS="$x11_CFLAGS $xmu_CFLAGS $gconf_CFLAGS"
		GNOME_LIBS="$x11_LIBS $xmu_LIBS $gconf_LIBS"
		AC_SUBST(GNOME_CFLAGS)
		AC_SUBST(GNOME_LIBS)
	else
		echo "GNOME module requires: x11, xmu and gconf!"
		exit 1
	fi
else
	with_gnome=no
fi
AM_CONDITIONAL([WITH_GNOME], [test x$with_gnome = xyes])

# KDE
AC_ARG_WITH([kde],
            [AS_HELP_STRING([--with-kde],
            [build KDE configuration module @<:@automatic@:>@])],
            [],
            [test x$have_x11   = xyes &&
             test x$have_xmu   = xyes &&
             with_kde=yes])
if test x$with_kde = xyes; then
	if test x$have_x11 = xyes && \
	   test x$have_xmu = xyes; then
		KDE_CFLAGS="$x11_CFLAGS $xmu_CFLAGS"
		KDE_LIBS="$x11_LIBS $xmu_LIBS"
		AC_SUBST(KDE_CFLAGS)
		AC_SUBST(KDE_LIBS)
	else
		echo "KDE module requires: x11 and xmu!"
		exit 1
	fi
else
	with_kde=no
fi
AM_CONDITIONAL([WITH_KDE], [test x$with_kde = xyes])

# WPAD
AC_ARG_WITH([wpad],
            [AS_HELP_STRING([--with-wpad],
            [build WPAD configuration module @<:@default=yes@:>@])],
            [], [with_wpad=yes])
AM_CONDITIONAL([WITH_WPAD], [test x$with_wpad = xyes])

# NetworkManager
AC_ARG_WITH([networkmanager],
            [AS_HELP_STRING([--with-networkmanager],
            [build NetworkManager module @<:@automatic@:>@])],
            [],
            [test x$have_networkmanager = xyes && with_networkmanager=yes])
if test x$with_networkmanager = xyes; then
	if test x$have_networkmanager = xyes; then
		NETWORKMANAGER_CFLAGS="$NetworkManager_CFLAGS"
		NETWORKMANAGER_LIBS="$NetworkManager_LIBS"
		AC_SUBST(NETWORKMANAGER_CFLAGS)
		AC_SUBST(NETWORKMANAGER_LIBS)
	else
		echo "NetworkManager module requires: NetworkManager!"
		exit 1
	fi
else
	with_networkmanager=no
fi
AM_CONDITIONAL([WITH_NETWORKMANAGER], [test x$with_networkmanager = xyes])

# Mozilla Javascript
AC_ARG_WITH([mozjs],
            [AS_HELP_STRING([--with-mozjs],
            [build Mozilla JavaScript PAC runner module @<:@automatic@:>@])],
            [],
            [test x$have_mozjs = xyes && with_mozjs=yes])
if test x$with_mozjs = xyes; then
	if test x$have_mozjs = xyes; then
		MOZJS_CFLAGS="$mozjs_CFLAGS"
		MOZJS_LIBS="$mozjs_LIBS"
		AC_SUBST(MOZJS_CFLAGS)
		AC_SUBST(MOZJS_LIBS)
	else
		echo "Mozilla JavaScript module requires: mozjs!"
		exit 1
	fi
else
	with_mozjs=no
fi
AM_CONDITIONAL([WITH_MOZJS], [test x$with_mozjs = xyes])

# WebKit Javascript
AC_ARG_WITH([webkit],
            [AS_HELP_STRING([--with-webkit],
            [build WebKit JavaScriptCore PAC runner module @<:@automatic@:>@])],
            [],
            [test x$have_webkit = xyes && with_webkit=yes])
if test x$with_webkit = xyes; then
	if test x$have_webkit = xyes; then
		WEBKIT_CFLAGS="$webkit_CFLAGS"
		WEBKIT_LIBS="$webkit_LIBS"
		AC_SUBST(WEBKIT_CFLAGS)
		AC_SUBST(WEBKIT_LIBS)
	else
		echo "WebKit JavaScriptCore module requires: WebKit!"
		exit 1
	fi
else
	with_webkit=no
fi
AM_CONDITIONAL([WITH_WEBKIT], [test x$with_webkit = xyes])

### Check for binding requirements
# Python
AC_ARG_WITH([python],
            [AS_HELP_STRING([--with-python],
            [build Python bindings @<:@automatic@:>@])],
            [AM_PATH_PYTHON([2.5], have_python=yes, have_python=no)],
            [have_python=yes && with_python=yes])
AM_CONDITIONAL([WITH_PYTHON], [test x$with_python = xyes])

# Java
with_java=no
AM_CONDITIONAL([WITH_JAVA], [test x$with_java = xyes])

# .NET
AC_ARG_WITH([dotnet],
            [AS_HELP_STRING([--with-dotnet],
            [build .NET/Mono bindings @<:@automatic@:>@])],
            [PKG_CHECK_MODULES(MONO,mono >= 2.0.0, have_dotnet=yes, have_dotnet=no)],
            [test x$have_dotnet = xyes && with_dotnet=yes])
if test x$with_dotnet = xyes; then
    if test x$have_dotnet = xyes; then
	    AC_PATH_PROG(MCS, gmcs)
        AC_PATH_PROG(GACUTIL, gacutil)
        ASSEMBLY_NAME="$PACKAGE-sharp"
        ASSEMBLY_VERSION="$VERSION"
        DOTNET_CFLAGS="$dotnet_CFLAGS"
        DOTNET_LIBS="$dotnet_LIBS"
        AC_SUBST(DOTNET_CFLAGS)
        AC_SUBST(DOTNET_LIBS)
        AC_SUBST(ASSEMBLY_NAME)
        AC_SUBST(ASSEMBLY_VERSION)
    else
        echo ".Net / Mono mindings requires: mono!"
        exit 1
    fi
else
	with_dotnet=no
fi
AM_CONDITIONAL([WITH_DOTNET], [test x$with_dotnet = xyes])

### Checks for header files.
AC_HEADER_STDC
#AC_CHECK_HEADERS([netdb.h stdlib.h string.h sys/socket.h unistd.h])

### Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
MODULEDIR=$libdir/$PACKAGE_NAME/$PACKAGE_VERSION/modules
AC_SUBST(MODULEDIR)
AC_SUBST(SYSCONFDIR)
AC_SUBST(DIR_SEPARATOR)

### Checks for library functions.
#AC_FUNC_MALLOC
#AC_CHECK_FUNCS([gethostbyname gethostname memset socket strdup strstr])

### OS Specific checks
AM_CONDITIONAL([MINGW32], [test "x$host_os" = xmingw32])
case $host in
  *-*-mingw*)
     AC_CHECK_LIB(ws2_32, main)
esac

CFLAGS="-g -Wall -pedantic $CFLAGS"
AC_CONFIG_FILES([libproxy-1.0.pc Makefile libproxy.spec src/Makefile src/lib/Makefile
                 src/modules/Makefile src/bin/Makefile src/bindings/Makefile
                 src/bindings/python/Makefile src/bindings/java/Makefile
                 src/bindings/dotnet/Makefile src/bindings/dotnet/libproxy-sharp-1.0.pc
                 src/bindings/dotnet/AssemblyInfo.cs])
AC_OUTPUT

### Print build summary
echo
echo "------------------------------------------------------"
echo -e "\tModules to build..."
echo -e "\t\tdirect          : $with_direct"
echo -e "\t\tenvvar          : $with_envvar"
echo -e "\t\tfile            : $with_file"
echo -e "\t\tgnome           : $with_gnome"
echo -e "\t\tkde             : $with_kde"
echo -e "\t\twpad            : $with_wpad"
echo -e "\t\tnetworkmanager  : $with_networkmanager"
echo -e "\t\tmozjs           : $with_mozjs"
echo -e "\t\twebkit          : $with_webkit"
echo
echo -e "\tBindings to build..."
echo -e "\t\tpython          : $with_python"
echo -e "\t\tjava            : $with_java"
echo -e "\t\tdotnet          : $with_dotnet"
echo "------------------------------------------------------"
echo

