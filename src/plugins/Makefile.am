plugindir          = $(libdir)/$(PACKAGE_NAME)/$(PACKAGE_VERSION)/plugins

plugin_LTLIBRARIES = envvar.la

# Environmental Variable Config Plugin
envvar_la_SOURCES  = envvar.c ../lib/proxy_plugin.h
envvar_la_CFLAGS   = -I../lib
envvar_la_LIBADD   = ../lib/libproxy.la
envvar_la_LDFLAGS  = -module -avoid-version

install-data-hook:
	rm -f $(DESTDIR)$(plugindir)/*.a $(DESTDIR)$(plugindir)/*.la