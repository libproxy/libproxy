### Required
cmake_minimum_required(VERSION 2.8)
project(libproxy)


### Globals
# Set the version
set(CMAKE_PROJECT_VERSION 0.4.0)

# Setup our relative paths
set(rlibdir lib${LIB_SUFFIX}/)
set(rmoduledir  ${rlibdir}/${CMAKE_PROJECT_NAME}/${CMAKE_PROJECT_VERSION}/modules/)
set(rlibexecdir libexec/)
set(rbindir     bin/)
set(rincludedir include/)

# Setup our full paths
set(prefix     ${CMAKE_INSTALL_PREFIX})
set(libdir     ${prefix}/${rlibdir})
set(moduledir  ${prefix}/${rmoduledir})
set(libexecdir ${prefix}/${rlibexecdir})
set(bindir     ${prefix}/${rbindir})
set(includedir ${prefix}/${rincludedir})

# Add a global compilation stuff
add_definitions(-D_POSIX_C_SOURCE=1)
set(CMAKE_CXX_FLAGS "-g -Wall -Werror ${CMAKE_CXX_FLAGS}")

### Subdirectories
add_subdirectory(libproxy)
add_subdirectory(bindings)
add_subdirectory(utils)
