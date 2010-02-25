### Required
cmake_minimum_required(VERSION 2.8)
project(libproxy)

### Enable packaging
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 4)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

set(CPACK_IGNORE_FILES "CMakeCache.txt;install_manifest.txt;utils/proxy$;/Testing/;/Makefile$;\\\\.cmake$;\\\\.tar.gz$;\\\\.so[.0-9]*$;/build/;/_CPack_Packages/;/CMakeFiles/;/CVS/;/\\\\.svn/;/\\\\.git/;\\\\.swp$;\\\\.#;/#")
set(CPACK_SOURCE_IGNORE_FILES ${CPACK_IGNORE_FILES})

SET(CPACK_SOURCE_TBZ2 "OFF")
SET(CPACK_SOURCE_TGZ "ON")
SET(CPACK_SOURCE_TZ "OFF")

SET(CPACK_SOURCE_PACKAGE_FILE_NAME "libproxy-${CPACK_PACKAGE_VERSION}")
SET(CPACK_SOURCE_TOPLEVEL_TAG "")

include(CPack)

### Globals
# Set the version
set(CMAKE_PROJECT_VERSION "${CPACK_PACKAGE_VERSION}")

# Enable testing
enable_testing()

# Setup our relative paths
if(${WIN32})
  set(rlibdir .)
  set(rmoduledir  modules/)
  set(rlibexecdir .)
  set(rbindir     .)
  set(rincludedir include/)
else()
  set(rlibdir lib${LIB_SUFFIX}/)
  set(rmoduledir  ${rlibdir}/${CMAKE_PROJECT_NAME}/${CMAKE_PROJECT_VERSION}/modules/)
  set(rlibexecdir libexec/)
  set(rbindir     bin/)
  set(rincludedir include/)
endif()

# Setup our full paths
set(prefix     ${CMAKE_INSTALL_PREFIX})
set(libdir     ${prefix}/${rlibdir})
set(moduledir  ${prefix}/${rmoduledir})
set(libexecdir ${prefix}/${rlibexecdir})
set(bindir     ${prefix}/${rbindir})
set(includedir ${prefix}/${rincludedir})

# Add a global compilation stuff
if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS=1)
else()
  add_definitions(-D_POSIX_C_SOURCE=1)
  set(CMAKE_CXX_FLAGS "-g -Wall -Werror -fvisibility=hidden ${CMAKE_CXX_FLAGS}")
endif()

### Subdirectories
add_subdirectory(libmodman)
add_subdirectory(libproxy)
add_subdirectory(bindings)
add_subdirectory(utils)
