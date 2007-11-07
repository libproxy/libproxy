plugindir           = @PLUGINDIR@

# Which modules to build
plugin_LTLIBRARIES  = envvar.la file.la
if WITH_MOZJS
plugin_LTLIBRARIES += mozjs.la
endif
if WITH_GNOME
plugin_LTLIBRARIES += gnome.la
endif
if WITH_NETWORKMANAGER
plugin_LTLIBRARIES += networkmanager.la
endif

# Environmental Variable Config Plugin
envvar_la_SOURCES         = envvar.c
envvar_la_CFLAGS          = -I../lib
envvar_la_LIBADD          = ../lib/libproxy.la
envvar_la_LDFLAGS         = -module -avoid-version

# File Config Plugin
file_la_SOURCES           = file.c
file_la_CFLAGS            = -I../lib
file_la_LIBADD            = ../lib/libproxy.la
file_la_LDFLAGS           = -module -avoid-version

# Mozilla (Spidermonkey) based PAC runner
mozjs_la_SOURCES          = mozjs.c
mozjs_la_CFLAGS           = -I../lib @MOZJS_CFLAGS@
mozjs_la_LIBADD           = ../lib/libproxy.la
mozjs_la_LDFLAGS          = -module -avoid-version @MOZJS_LIBS@
nodist_mozjs_la_SOURCES   = pacutils.h

# GConf/GNOME Config Plugin
gnome_la_SOURCES          = gnome.c
gnome_la_CFLAGS           = -I../lib @GCONF_CFLAGS@
gnome_la_LIBADD           = ../lib/libproxy.la
gnome_la_LDFLAGS          = -module -avoid-version @GCONF_LIBS@

# NetworkManager Plugin
networkmanager_la_SOURCES = networkmanager.c
networkmanager_la_CFLAGS  = -I../lib @NETWORKMANAGER_CFLAGS@
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
