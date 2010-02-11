### Main library
add_library(libmodman SHARED module.cpp module_manager.cpp)
if(NOT WIN32)
  target_link_libraries(libmodman dl)
endif()
set_target_properties(libmodman PROPERTIES PREFIX "" VERSION 0 SOVERSION 0.0.0)
install(TARGETS libmodman DESTINATION ${rlibdir})

### Tests
add_subdirectory(test)