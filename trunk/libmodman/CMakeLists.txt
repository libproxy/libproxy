### Main library
# Flags / Definitions / Environment
if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS=1)
else(WIN32)
  set(CMAKE_CXX_FLAGS "-fvisibility=hidden ${CMAKE_CXX_FLAGS}")
endif(WIN32)
include_directories(${CMAKE_SOURCE_DIR})

add_library(modman SHARED
            module.hpp
            module_manager.hpp
            module_manager.cpp)
if(NOT WIN32)
  target_link_libraries(modman dl)
endif()
set_target_properties(modman PROPERTIES PREFIX "lib" VERSION 1.0.0 SOVERSION 1)
install(TARGETS modman DESTINATION ${LIB_INSTALL_DIR})

### Tests
add_testdirectory(test)
