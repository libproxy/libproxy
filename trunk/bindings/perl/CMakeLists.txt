find_package(Perl)
include (FindPerlLibs)

if(PERL_FOUND AND PERLLIBS_FOUND)

  # Some distributions install perl packages in vendor when shipped with the distro.
  # Let's make their lifes easier by offering an install flag for this usecase.
  if(PERL_VENDORINSTALL)
    set (PX_PERL_ARCH ${PERL_VENDORARCH})
    set (PX_PERL_LIB ${PERL_VENDORLIB})
  else()
    # PERL_SITEARCH is actually not defined in latest FindPerlLibs.cmake macros, thus
    # we have to find the correct path on our own.
    EXECUTE_PROCESS(COMMAND ${PERL_EXECUTABLE} -MConfig -e "print \$Config{sitearch}"
      OUTPUT_VARIABLE PX_PERL_ARCH)
    set (PX_PERL_LIB ${PERL_SITELIB})
  endif()

  add_subdirectory(lib)
  add_subdirectory(src)
  add_subdirectory(t)
endif()

