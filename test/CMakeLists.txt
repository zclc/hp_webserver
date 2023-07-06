cmake_minimum_required(VERSION 3.12)

project(memory_test)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g")

add_executable(memory_test memory_pool.c 02_test_memorypool.c)

target_include_directories(memory_test PUBLIC ../src)
