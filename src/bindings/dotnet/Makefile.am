libproxydir=$(pkglibdir)
libproxy_SCRIPTS = libproxy-sharp.dll
CLEANFILES = libproxy-sharp.dll

libproxy_sources = libproxy.cs

libproxy_build_sources = $(addprefix $(srcdir)/, $(libproxy_sources))

libproxy-sharp.dll: $(libproxy_build_sources)
	$(MCS) /target:library -out:$@ $< -keyfile:libproxy.snk

