moduledir = @MODULEDIR@
AM_CFLAGS    = -std=c99
AM_LDFLAGS   = -module -avoid-version -no-undefined


# Which modules to build
module_LTLIBRARIES  = ignore_domain.la ignore_ip.la # config_w32reg.la
if WITH_DIRECT
module_LTLIBRARIES += config_direct.la
endif
if WITH_ENVVAR
module_LTLIBRARIES += config_envvar.la
endif
if WITH_FILE
module_LTLIBRARIES += config_file.la
endif
if WITH_GNOME
module_LTLIBRARIES += config_gnome.la
endif
if WITH_KDE4
module_LTLIBRARIES += config_kde4.la
endif
if WITH_WPAD
module_LTLIBRARIES += config_wpad.la wpad_dns.la wpad_dnsdevolution.la
endif
if WITH_NETWORKMANAGER
module_LTLIBRARIES += network_networkmanager.la
endif
if WITH_MOZJS
module_LTLIBRARIES += pacrunner_mozjs.la
endif
if WITH_WEBKIT
module_LTLIBRARIES += pacrunner_webkit.la
endif

# Direct Config Module
config_direct_la_SOURCES           = config_direct.c
config_direct_la_CPPFLAGS          = -I$(top_srcdir)/src/lib
config_direct_la_LIBADD            = ../lib/libproxy.la

# Environmental Variable Config Module
config_envvar_la_SOURCES           = config_envvar.c
config_envvar_la_CPPFLAGS          = -I$(top_srcdir)/src/lib
config_envvar_la_LIBADD            = ../lib/libproxy.la

# File Config Module
config_file_la_SOURCES             = config_file.c
config_file_la_CPPFLAGS            = -I$(top_srcdir)/src/lib -DSYSCONFDIR=\"@SYSCONFDIR@@DIR_SEPARATOR@\"
config_file_la_LIBADD              = ../lib/libproxy.la

# GConf/GNOME Config Module
config_gnome_la_SOURCES            = config_gnome.c xhasclient.c
config_gnome_la_CPPFLAGS           = -I$(top_srcdir)/src/lib -D_POSIX_C_SOURCE=200112L -DGCONFTOOLBIN='"@GCONFTOOLBIN@"' @GNOME_CFLAGS@
config_gnome_la_LIBADD             = ../lib/libproxy.la
config_gnome_la_LDFLAGS            = $(AM_LDFLAGS) @GNOME_LIBS@

# KDE4 Config Module
config_kde4_la_SOURCES              = config_kde.cc
config_kde4_la_CPPFLAGS             = -I$(top_srcdir)/src/lib @KDE4_CFLAGS@
config_kde4_la_LIBADD               = ../lib/libproxy.la
config_kde4_la_LDFLAGS              = $(AM_LDFLAGS) @KDE4_LIBS@ -lkdecore

# Windows Registry Config Module
config_w32reg_la_SOURCES           = config_w32reg.c
config_w32reg_la_CPPFLAGS          = -I$(top_srcdir)/src/lib
config_w32reg_la_LIBADD            = ../lib/libproxy.la

# WPAD Config Module
config_wpad_la_SOURCES             = config_wpad.c
config_wpad_la_CPPFLAGS            = -I$(top_srcdir)/src/lib
config_wpad_la_LIBADD              = ../lib/libproxy.la

# Domain Ignore Module
ignore_domain_la_SOURCES           = ignore_domain.c
ignore_domain_la_CPPFLAGS          = -I$(top_srcdir)/src/lib
ignore_domain_la_LIBADD            = ../lib/libproxy.la

# IP Ignore Module
ignore_ip_la_SOURCES               = ignore_ip.c
ignore_ip_la_CPPFLAGS              = -I$(top_srcdir)/src/lib -D_POSIX_C_SOURCE=200112L
ignore_ip_la_LIBADD                = ../lib/libproxy.la

# WebKit (JavaScriptCore) based PAC runner
pacrunner_webkit_la_SOURCES        = pacrunner_webkit.c
pacrunner_webkit_la_CPPFLAGS       = -I$(top_srcdir)/src/lib @WEBKIT_CFLAGS@ -D_POSIX_C_SOURCE=200112L
pacrunner_webkit_la_LIBADD         = ../lib/libproxy.la
if MINGW32
pacrunner_webkit_la_LIBADD	  += ../platform/win32/libwinutils.la
endif
pacrunner_webkit_la_LDFLAGS        = $(AM_LDFLAGS) @WEBKIT_LIBS@
nodist_pacrunner_webkit_la_SOURCES = pacutils.h

# Mozilla (Spidermonkey) based PAC runner
pacrunner_mozjs_la_SOURCES         = pacrunner_mozjs.c
pacrunner_mozjs_la_CPPFLAGS        = -I$(top_srcdir)/src/lib @MOZJS_CFLAGS@ -D_POSIX_C_SOURCE=200112L
pacrunner_mozjs_la_LIBADD          = ../lib/libproxy.la
pacrunner_mozjs_la_LDFLAGS         = $(AM_LDFLAGS) @MOZJS_LIBS@
nodist_pacrunner_mozjs_la_SOURCES  = pacutils.h

# NetworkManager Module
network_networkmanager_la_SOURCES  = network_networkmanager.c
network_networkmanager_la_CPPFLAGS = -I$(top_srcdir)/src/lib @NETWORKMANAGER_CFLAGS@
network_networkmanager_la_LIBADD   = ../lib/libproxy.la
network_networkmanager_la_LDFLAGS  = $(AM_LDFLAGS) @NETWORKMANAGER_LIBS@

# DNS WPAD Module
wpad_dns_la_SOURCES                = wpad_dns.c
wpad_dns_la_CPPFLAGS               = -I$(top_srcdir)/src/lib
wpad_dns_la_LIBADD                 = ../lib/libproxy.la

# DNS WPAD Module
wpad_dnsdevolution_la_SOURCES      = wpad_dnsdevolution.c
wpad_dnsdevolution_la_CPPFLAGS     = -I$(top_srcdir)/src/lib
wpad_dnsdevolution_la_LIBADD       = ../lib/libproxy.la

BUILT_SOURCES = pacutils.h
CLEANFILES = pacutils.h
EXTRA_DIST = pacutils.js

pacutils.h: pacutils.js
	echo "#define JAVASCRIPT_ROUTINES \\" > $@
	cpp $< | grep -v '^#' | grep -v '^$$' | sed 's|^\(.*\)$$|"\1\\n" \\|' >> $@
	echo '""' >> $@

install-data-hook:
	rm -f $(DESTDIR)$(moduledir)/*.a $(DESTDIR)$(moduledir)/*.la
	
uninstall-hook:
	rm -f $(DESTDIR)$(moduledir)/*.so
