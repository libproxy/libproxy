lib_LTLIBRARIES     = libproxy.la
libproxy_la_SOURCES = \
						array.c array.h \
						config_file.c config_file.h \
						misc.c misc.h \
						module_manager.c module_manager.h \
						modules.h \
						pac.c pac.h \
						proxy.c proxy.h \
						strdict.c strdict.h \
						url.c url.h

libproxy_la_CFLAGS  = -DMODULEDIR=\"@MODULEDIR@@DIR_SEPARATOR@\" -DSYSCONFDIR=\"@SYSCONFDIR@@DIR_SEPARATOR@\" -std=c99 -D_POSIX_C_SOURCE=200112L
libproxy_la_LDFLAGS = -lpthread -lm -no-undefined

include_HEADERS     = proxy.h

