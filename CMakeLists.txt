cmake_minimum_required(VERSION 2.8)

project (WebServer)

SET(CMAKE_SYSTEM_NAME Linux)  

include_directories( app/include/ )

add_definitions(-D_HOEL_SQLITE=1)

file(GLOB headers "app/include/*.h" )
file(GLOB sources "app/src/*.c" )

add_executable(web-server ${sources} ${headers})

target_link_libraries( web-server pthread sqlite3 ulfius orcania jansson microhttpd jansson m )
