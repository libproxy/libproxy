### Required
cmake_minimum_required(VERSION 2.6)
if(POLICY CMP0011)
   cmake_policy(SET CMP0011 NEW)
endif(POLICY CMP0011)

### Make sure we have a build type
# This fixes an install issue where installs fail
# when find_package(KDE4) is present.  I'm still
# not sure why this happens, but this definitely fixes
# it.  It also fixes tests not running in this case.
# See issues 127 and 128.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
  set(CMAKE_INSTALL_CONFIG_NAME RelWithDebInfo)
else()
  set(CMAKE_INSTALL_CONFIG_NAME ${CMAKE_BUILD_TYPE})
endif()

include(cmake/cpack.cmk)
include(cmake/ctest.cmk)
include(cmake/paths.cmk)

### Subdirectories
# Conditionally build bundled libmodman
find_package(libmodman QUIET)
if(LIBMODMAN_FOUND)
    message(STATUS "Building with system libmodman")    
else()
    message(STATUS "Building with bundled libmodman")
    add_subdirectory(libmodman)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()

# Always build these
add_subdirectory(libproxy)
add_subdirectory(bindings)
add_subdirectory(utils)

