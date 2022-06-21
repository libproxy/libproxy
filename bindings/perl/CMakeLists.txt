find_package(Perl)
include (FindPerlLibs)

if(PERL_FOUND AND PERLLIBS_FOUND)

  # Enable override of perl packge install directory.
  set(PX_PERL_ARCH ${PX_PERL_ARCH} CACHE PATH "Install directory for Perl package.")

  if(NOT PX_PERL_ARCH)
    # Some distributions install perl packages in vendor when shipped with the distro.
    # Let's make their lifes easier by offering an install flag for this usecase.
    option(PERL_VENDORINSTALL "Install Perl package in vendor directory" OFF)

    # Offer an option to explicitly link against libperl.so, with a default
    # that depends on the value of the PERL_VENDORINSTALL option: if the
    # binding is installed into a version-independent directory such as
    # vendor_perl the chances of it needing a rebuild on each Perl update are
    # lower. Note: not linking against libperl.so does not work if
    # --no-undefined is passed to the linker.
    if(PERL_VENDORINSTALL)
        option(PERL_LINK_LIBPERL "Explicitly link against libperl.so" OFF)
    else()
        option(PERL_LINK_LIBPERL "Explicitly link against libperl.so" ON)
    endif()

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

    # PERL_EXTRA_C_FLAGS is not complete, we need full flags (including e.g.
    # _FILE_OFFSET_BITS=64) for compatibility.
    EXECUTE_PROCESS(COMMAND ${PERL_EXECUTABLE} -MConfig -e "print \$Config{ccflags}"
      OUTPUT_VARIABLE PX_PERL_CCFLAGS)
    # Convert it to a "list" suitable for target_compile_options.
    string(REPLACE " " ";" PX_PERL_CCFLAGS ${PX_PERL_CCFLAGS})
  endif()

  add_subdirectory(lib)
  add_subdirectory(src)
  add_subdirectory(t)
endif()

