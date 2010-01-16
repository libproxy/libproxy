include_directories("../libproxy")

### Executables
add_executable(proxy proxy.c)
target_link_libraries(proxy libproxy)
install(TARGETS proxy RUNTIME DESTINATION ${rbindir})

add_executable(iniparse iniparse.c)
target_link_libraries(iniparse libproxy)

set_target_properties(proxy iniparse PROPERTIES COMPILE_FLAGS "-g -std=c99")

