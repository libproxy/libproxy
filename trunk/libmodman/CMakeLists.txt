### Main library
add_library(modman SHARED module.hpp module_manager.hpp module_manager.cpp)
if(NOT WIN32)
  target_link_libraries(modman dl)
endif()
set_target_properties(modman PROPERTIES PREFIX "lib" VERSION 0.0.0 SOVERSION 1)
install(TARGETS modman DESTINATION ${LIB_INSTALL_DIR})

### Tests
if (BUILD_TESTING)
  add_subdirectory (test)
endif (BUILD_TESTING)
