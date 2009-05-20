libproxydir=$(pkglibdir)
libproxy_SCRIPTS = libproxy.dll
CLEANFILES = libproxy.dll

libproxy_sources = libproxy.cs

libproxy_build_sources = $(addprefix $(srcdir)/, $(libproxy_sources))

libproxy.dll: $(libproxy_build_sources)
	$(MCS) /target:library -out:$@ $< 

