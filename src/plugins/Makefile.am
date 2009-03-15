plugindir           = @PLUGINDIR@

# Which modules to build
plugin_LTLIBRARIES  = ignore_domain.la
if WITH_DIRECT
plugin_LTLIBRARIES += config_direct.la
endif
if WITH_ENVVAR
plugin_LTLIBRARIES += config_envvar.la
endif
if WITH_FILE
plugin_LTLIBRARIES += config_file.la
endif
if WITH_GNOME
plugin_LTLIBRARIES += config_gnome.la
endif
if WITH_KDE
plugin_LTLIBRARIES += config_kde.la
endif
if WITH_WPAD
plugin_LTLIBRARIES += config_wpad.la
endif
if WITH_NETWORKMANAGER
plugin_LTLIBRARIES += network_networkmanager.la
endif
if WITH_MOZJS
plugin_LTLIBRARIES += pacrunner_mozjs.la
endif
if WITH_WEBKIT
plugin_LTLIBRARIES += pacrunner_webkit.la
endif

# Direct Config Plugin
config_direct_la_SOURCES           = config_direct.c
config_direct_la_CFLAGS            = -I$(top_srcdir)/src/lib
config_direct_la_LIBADD            = ../lib/libproxy.la
config_direct_la_LDFLAGS           = -module -avoid-version

# Environmental Variable Config Plugin
config_envvar_la_SOURCES           = config_envvar.c
config_envvar_la_CFLAGS            = -I$(top_srcdir)/src/lib
config_envvar_la_LIBADD            = ../lib/libproxy.la
config_envvar_la_LDFLAGS           = -module -avoid-version

# File Config Plugin
config_file_la_SOURCES             = config_file.c
config_file_la_CFLAGS              = -I$(top_srcdir)/src/lib
config_file_la_LIBADD              = ../lib/libproxy.la
config_file_la_LDFLAGS             = -module -avoid-version

# GConf/GNOME Config Plugin
config_gnome_la_SOURCES            = config_gnome.c xhasclient.c
config_gnome_la_CFLAGS             = -I$(top_srcdir)/src/lib @GNOME_CFLAGS@
config_gnome_la_LIBADD             = ../lib/libproxy.la
config_gnome_la_LDFLAGS            = -module -avoid-version @GNOME_LIBS@

# KDE Config Plugin
config_kde_la_SOURCES              = config_kde.c xhasclient.c
config_kde_la_CFLAGS               = -I$(top_srcdir)/src/lib @KDE_CFLAGS@
config_kde_la_LIBADD               = ../lib/libproxy.la
config_kde_la_LDFLAGS              = -module -avoid-version @KDE_LIBS@

# WPAD Config Plugin
config_wpad_la_SOURCES             = config_wpad.c
config_wpad_la_CFLAGS              = -I$(top_srcdir)/src/lib
config_wpad_la_LIBADD              = ../lib/libproxy.la
config_wpad_la_LDFLAGS             = -module -avoid-version

# Domain Ignore Plugin
ignore_domain_la_SOURCES           = ignore_domain.c
ignore_domain_la_CFLAGS            = -I$(top_srcdir)/src/lib
ignore_domain_la_LIBADD            = ../lib/libproxy.la
ignore_domain_la_LDFLAGS           = -module -avoid-version

# WebKit (JavaScriptCore) based PAC runner
pacrunner_webkit_la_SOURCES        = pacrunner_webkit.c
pacrunner_webkit_la_CFLAGS         = -I$(top_srcdir)/src/lib @WEBKIT_CFLAGS@
pacrunner_webkit_la_LIBADD         = ../lib/libproxy.la
pacrunner_webkit_la_LDFLAGS        = -module -avoid-version @WEBKIT_LIBS@
nodist_pacrunner_webkit_la_SOURCES = pacutils.h

# Mozilla (Spidermonkey) based PAC runner
pacrunner_mozjs_la_SOURCES         = pacrunner_mozjs.c
pacrunner_mozjs_la_CFLAGS          = -I$(top_srcdir)/src/lib @MOZJS_CFLAGS@
pacrunner_mozjs_la_LIBADD          = ../lib/libproxy.la
pacrunner_mozjs_la_LDFLAGS         = -module -avoid-version @MOZJS_LIBS@
nodist_pacrunner_mozjs_la_SOURCES  = pacutils.h

# NetworkManager Plugin
network_networkmanager_la_SOURCES  = network_networkmanager.c
network_networkmanager_la_CFLAGS   = -I$(top_srcdir)/src/lib @NETWORKMANAGER_CFLAGS@
network_networkmanager_la_LIBADD   = ../lib/libproxy.la
network_networkmanager_la_LDFLAGS  = -module -avoid-version @NETWORKMANAGER_LIBS@

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
