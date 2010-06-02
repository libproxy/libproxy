####
## Libproxy Tests
####

include_directories(../)

# URL Test
add_executable(url-test url-test.cpp ../url.cpp)
add_test(NAME url-test COMMAND url-test)

# URL Encode
add_executable(url-encode url-encode.cpp ../url.cpp)
add_test(NAME url-encode COMMAND url-encode)

