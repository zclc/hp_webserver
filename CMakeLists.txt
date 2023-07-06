cmake_minimum_required(VERSION 3.12)
project (main)



add_subdirectory("zlog")

file(GLOB SOURCES "src/*.c")

add_executable(main ${SOURCES})

target_link_libraries(main PUBLIC pthread zlog)