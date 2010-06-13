include_directories("../libproxy")
link_directories(${LIBPROXY_LIBRARY_DIRS})

add_executable(proxy proxy.c)
target_link_libraries(proxy libproxy)
install(TARGETS proxy RUNTIME DESTINATION ${BIN_INSTALL_DIR})
