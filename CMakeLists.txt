cmake_minimum_required(VERSION 3.12)
project (main)

file(GLOB SOURCES "src/*.c")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-g -pthread -DNDEBUG")
add_executable(main ${SOURCES})

