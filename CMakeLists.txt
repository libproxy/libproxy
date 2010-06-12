### Required
cmake_minimum_required(VERSION 2.6)

include(cmake/cpack.cmk)
include(cmake/ctest.cmk)
include(cmake/paths.cmk)

### Subdirectories
# Conditionally build bundled libmodman
find_package(libmodman)
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

