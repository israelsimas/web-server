cmake_minimum_required(VERSION 2.8)

project (WebServer)

SET(CMAKE_SYSTEM_NAME Linux)  

include_directories( app/include/ )

if(APPLE)
  include_directories( /usr/local/include )
  link_directories( /usr/local/lib/ )
  set(CMAKE_MACOSX_RPATH ON)
endif()

# add_definitions(-DPLATFORM_X86=1)

file(GLOB headers "app/include/*.h" )
file(GLOB sources "app/src/*.c" )

add_executable(web-server ${sources} ${headers})

target_link_libraries( web-server pthread sqlite3 utils iniparser middleware database network ulfius orcania jansson microhttpd jansson m )
