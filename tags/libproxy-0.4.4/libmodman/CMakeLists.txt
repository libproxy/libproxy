### Main library
add_library(libmodman SHARED module.hpp module_manager.hpp module_manager.cpp)
if(NOT WIN32)
  target_link_libraries(libmodman dl)
endif()
set_target_properties(libmodman PROPERTIES PREFIX "" VERSION 0.0.0 SOVERSION 0)
install(TARGETS libmodman DESTINATION ${LIB_INSTALL_PREFIX})

### Tests
if (BUILD_TESTING)
  add_subdirectory (test)
endif (BUILD_TESTING)
