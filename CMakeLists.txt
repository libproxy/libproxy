### Required
cmake_minimum_required(VERSION 2.6)

project(libproxy)

if(POLICY CMP0011)
   cmake_policy(SET CMP0011 NEW)
endif(POLICY CMP0011)
if(POLICY CMP0054)
   cmake_policy(SET CMP0054 NEW)
endif(POLICY CMP0054)

# Make sure we look in our cmake folder for additional definitions
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake )

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

option(WITH_WEBKIT3 "Build against gtk-3 version of webkitgtk" OFF)

### Subdirectories

add_subdirectory(libmodman)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Conditionally build bindings
if(NOT WIN32)
    add_subdirectory(bindings)
endif()

# Always build these
add_subdirectory(libproxy)
add_subdirectory(utils)

