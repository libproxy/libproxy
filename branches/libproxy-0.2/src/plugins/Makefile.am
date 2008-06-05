plugindir           = @PLUGINDIR@

# Which modules to build
plugin_LTLIBRARIES  =
if WITH_ENVVAR
plugin_LTLIBRARIES += envvar.la
endif
if WITH_FILE
plugin_LTLIBRARIES += file.la
endif
if WITH_GNOME
plugin_LTLIBRARIES += gnome.la
endif
if WITH_KDE
plugin_LTLIBRARIES += kde.la
endif
if WITH_WEBKIT
plugin_LTLIBRARIES += webkit.la
endif
if WITH_MOZJS
plugin_LTLIBRARIES += mozjs.la
endif
if WITH_NETWORKMANAGER
plugin_LTLIBRARIES += networkmanager.la
endif

# Environmental Variable Config Plugin
envvar_la_SOURCES         = envvar.c
envvar_la_CFLAGS          = -I$(top_srcdir)/src/lib
envvar_la_LIBADD          = ../lib/libproxy.la
envvar_la_LDFLAGS         = -module -avoid-version

# File Config Plugin
file_la_SOURCES           = file.c
file_la_CFLAGS            = -I$(top_srcdir)/src/lib
file_la_LIBADD            = ../lib/libproxy.la
file_la_LDFLAGS           = -module -avoid-version

# GConf/GNOME Config Plugin
gnome_la_SOURCES          = gnome.c xhasclient.c
gnome_la_CFLAGS           = -I$(top_srcdir)/src/lib @GNOME_CFLAGS@
gnome_la_LIBADD           = ../lib/libproxy.la
gnome_la_LDFLAGS          = -module -avoid-version @GNOME_LIBS@

# KDE Config Plugin
kde_la_SOURCES            = kde.c xhasclient.c
kde_la_CFLAGS             = -I$(top_srcdir)/src/lib @KDE_CFLAGS@
kde_la_LIBADD             = ../lib/libproxy.la
kde_la_LDFLAGS            = -module -avoid-version @KDE_LIBS@

# WebKit (JavaScriptCore) based PAC runner
webkit_la_SOURCES          = webkit.c
webkit_la_CFLAGS           = -I$(top_srcdir)/src/lib @WEBKIT_CFLAGS@
webkit_la_LIBADD           = ../lib/libproxy.la
webkit_la_LDFLAGS          = -module -avoid-version @WEBKIT_LIBS@
nodist_webkit_la_SOURCES   = pacutils.h

# Mozilla (Spidermonkey) based PAC runner
mozjs_la_SOURCES          = mozjs.c
mozjs_la_CFLAGS           = -I$(top_srcdir)/src/lib @MOZJS_CFLAGS@
mozjs_la_LIBADD           = ../lib/libproxy.la
mozjs_la_LDFLAGS          = -module -avoid-version @MOZJS_LIBS@
nodist_mozjs_la_SOURCES   = pacutils.h

# NetworkManager Plugin
networkmanager_la_SOURCES = networkmanager.c
networkmanager_la_CFLAGS  = -I$(top_srcdir)/src/lib @NETWORKMANAGER_CFLAGS@
networkmanager_la_LIBADD  = ../lib/libproxy.la
networkmanager_la_LDFLAGS = -module -avoid-version @NETWORKMANAGER_LIBS@

BUILT_SOURCES = pacutils.h
CLEANFILES = pacutils.h
EXTRA_DIST = pacutils.js

pacutils.h: pacutils.js
	echo "#define JAVASCRIPT_ROUTINES \\" > $@
	cpp $< | grep -v '^#' | grep -v '^$$' | sed 's|^\(.*\)$$|"\1\\n" \\|' >> $@
	echo '""' >> $@

install-data-hook:
	rm -f $(DESTDIR)$(plugindir)/*.a $(DESTDIR)$(plugindir)/*.la
	
uninstall-hook:
	rm -f $(DESTDIR)$(plugindir)/*.so
