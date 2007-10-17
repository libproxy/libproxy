plugindir           = @PLUGINDIR@

# Which modules to build
plugin_LTLIBRARIES  = envvar.la
if WITH_MOZJS
plugin_LTLIBRARIES += mozjs.la
endif
if WITH_GNOME
plugin_LTLIBRARIES += gnome.la
endif

# Environmental Variable Config Plugin
envvar_la_SOURCES       = envvar.c ../lib/proxy_plugin.h
envvar_la_CFLAGS        = -I../lib
envvar_la_LIBADD        = ../lib/libproxy.la
envvar_la_LDFLAGS       = -module -avoid-version

# Mozilla (Spidermonkey) based PAC runner
mozjs_la_SOURCES        = mozjs.c ../lib/proxy_plugin.h
mozjs_la_CFLAGS         = -I../lib @MOZJS_CFLAGS@
mozjs_la_LIBADD         = ../lib/libproxy.la
mozjs_la_LDFLAGS        = -module -avoid-version @MOZJS_LIBS@
nodist_mozjs_la_SOURCES = pacutils.h

# GConf/GNOME Config Plugin
gnome_la_SOURCES        = gnome.c ../lib/proxy_plugin.h
gnome_la_CFLAGS         = -I../lib @GCONF_CFLAGS@ @GDK_CFLAGS@
gnome_la_LIBADD         = ../lib/libproxy.la
gnome_la_LDFLAGS        = -module -avoid-version @GCONF_LIBS@ @GDK_LIBS@

BUILT_SOURCES = pacutils.h
CLEANFILES = pacutils.h
EXTRA_DIST = pacutils.js

pacutils.h: pacutils.js
	echo "#define JAVASCRIPT_ROUTINES \\" > $@
	cpp $< | grep -v '^#' | grep -v '^$$' | sed 's|^\(.*\)$$|"\1\\n" \\|' >> $@
	echo '""' >> $@

install-data-hook:
	rm -f $(DESTDIR)$(plugindir)/*.a $(DESTDIR)$(plugindir)/*.la
