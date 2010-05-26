####
## Libproxy Tests
####

include_directories(../)

# URL Test
add_executable(url-test url-test.cpp ../url.cpp)
add_test(NAME url-test COMMAND url-test)

