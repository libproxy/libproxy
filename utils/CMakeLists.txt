include_directories("../libproxy")

add_executable(proxy proxy.c)

if(BUILD_SHARED_LIBS)
  target_link_libraries(proxy libproxy)
else ()
  if(WIN32)
    target_link_libraries(proxy libproxy;modman;ws2_32;${LIBPROXY_LIBRARIES})
  else()
    target_link_libraries(proxy libproxy;modman;m;pthread;${CMAKE_DL_LIBS};${LIBPROXY_LIBRARIES})
  endif() 
endif()


install(TARGETS proxy RUNTIME DESTINATION ${BIN_INSTALL_DIR})
