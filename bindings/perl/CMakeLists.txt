find_package(Perl)
include (FindPerlMore)

if(PERL_FOUND)
  add_subdirectory(lib)
  add_subdirectory(src)
  add_subdirectory(t)
endif()

