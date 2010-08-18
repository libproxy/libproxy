### Project info
project(libproxy)
set_project_version(0 4 4)

### Add a global compilation stuff
if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS=1)
else()
  add_definitions(-D_POSIX_C_SOURCE=1)
  set(CMAKE_CXX_FLAGS "-g -Wall -fvisibility=hidden ${CMAKE_CXX_FLAGS}")
endif()

### Modules
include(cmake/modules.cmk)

### Dev files
include(cmake/devfiles.cmk)

### Main library
include(cmake/libproxy.cmk)

### Tests
# These tests don't seem to run with "make test"... Why?
add_testdirectory(test)
