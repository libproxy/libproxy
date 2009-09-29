bin_PROGRAMS  = proxy
noinst_PROGRAMS = iniparse.test

# Command line interface to libproxy
proxy_SOURCES = proxy.c
proxy_CPPFLAGS  = -I$(top_srcdir)/src/lib -std=c99
proxy_LDADD   = ../lib/libproxy.la

# a simple test routine parsing ini files.
iniparse_test_SOURCES = iniparse.c
iniparse_test_CPPFLAGS = -I$(top_srcdir)/src/lib -std=c99
iniparse_test_LDADD = ../lib/libproxy_la-strdict.o ../lib/libproxy_la-array.o ../lib/libproxy_la-config_file.o ../lib/libproxy_la-misc.o
