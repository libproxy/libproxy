include_directories("../libproxy")

### Executables
add_executable(proxy proxy.c)
target_link_libraries(proxy libproxy)
install(TARGETS proxy RUNTIME DESTINATION ${rbindir})

set_target_properties(proxy PROPERTIES COMPILE_FLAGS "-g -std=c99")

