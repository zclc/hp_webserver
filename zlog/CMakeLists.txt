cmake_minimum_required(VERSION 3.12)

project (zlog)

file(GLOB SOURCES "src/*.c")

add_library(zlog STATIC ${SOURCES})