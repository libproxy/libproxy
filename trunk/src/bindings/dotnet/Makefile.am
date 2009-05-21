libproxydir=$(pkglibdir)/@VERSION@
libproxy_SCRIPTS = @ASSEMBLY_NAME@.dll
CLEANFILES = @ASSEMBLY_NAME@.dll

EXTRA_DIST = libproxy-sharp-1.0.pc.in
EXTRA_SCRIPTS = config.status

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libproxy-sharp-1.0.pc

CSC = $(MCS) $(MCS_FLAGS) $(CSFLAGS)

libproxy_sources = libproxy.cs
libproxy_sources_in = AssemblyInfo.cs.in
libproxy_generated_sources = $(libproxy_sources_in:.in=)
libproxy_build_sources = $(addprefix $(srcdir)/, $(libproxy_sources))

@ASSEMBLY_NAME@.dll: $(libproxy_build_sources) $(libproxy_generated_sources)
	$(CSC) /target:library -out:$@ $< $(libproxy_generated_sources)

install-data-local:
	@if test -n '$(ASSEMBLY_NAME).dll'; then						\
	  echo "$(GACUTIL) /i $(ASSEMBLY_NAME).dll /f $(GACUTIL_FLAGS)";    \
	  $(GACUTIL) /i $(ASSEMBLY_NAME).dll /f $(GACUTIL_FLAGS) || exit 1;          \
	  for POLICY in $(POLICIES);                                        \
	    do echo "$(GACUTIL) /i $$POLICY /f $(GACUTIL_POLICY_FLAGS)";    \
	    $(GACUTIL) /i $$POLICY /f $(GACUTIL_POLICY_FLAGS) || exit 1;    \
	  done									\
	fi

uninstall-local:
	@if test -n '$(ASSEMBLY_NAME).dll'; then						\
	  echo "$(GACUTIL) /u $(ASSEMBLY_NAME) $(GACUTIL_FLAGS)";		\
	  $(GACUTIL) /u $(ASSEMBLY_NAME) $(GACUTIL_FLAGS) || exit 1;		\
	  for POLICY in $(taglib_policy_names);					\
	    do echo "$(GACUTIL) /u $$POLICY /f $(GACUTIL_POLICY_FLAGS)";	\
	    $(GACUTIL) /u $$POLICY /f $(GACUTIL_POLICY_FLAGS) || exit 1;	\
	  done									\
	fi

