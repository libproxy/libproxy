
include_directories( ${PERL_INCLUDE_PATH} ${CMAKE_SOURCE_DIR}/libproxy )
link_directories(${LIBPROXY_LIBRARY_DIRS})

# Run doxsubpp.pl to run xsubpp on Libproxy.xs
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Libproxy.c
                   COMMAND ${PERL_EXECUTABLE} ARGS ${CMAKE_CURRENT_SOURCE_DIR}/doxsubpp.pl
                           ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Libproxy.xs ${CMAKE_CURRENT_BINARY_DIR}/Libproxy.c
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Libproxy.xs libproxy
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set(Libproxy_LIB_SRCS Libproxy.c)

set(CMAKE_C_COMPILER ${CMAKE_CXX_COMPILER})
set(CMAKE_C_FLAGS ${PERL_CXX_FLAGS} )

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/perl/blib/arch/auto/Net)
add_library(PLlibproxy SHARED ${Libproxy_LIB_SRCS})

target_link_libraries(PLlibproxy ${PERL_LIBRARY} libproxy)
set_target_properties(PLlibproxy PROPERTIES OUTPUT_NAME "Libproxy")
set_target_properties(PLlibproxy PROPERTIES PREFIX "")

install( TARGETS PLlibproxy DESTINATION ${PX_PERL_ARCH}/auto/Net/Libproxy )
