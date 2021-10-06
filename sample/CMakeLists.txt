cmake_minimum_required(VERSION 3.0)
set(CMAKE_VERBOSE_MAKEFILE ON)
project(wsocket)

file(GLOB srcs *.c ../*.c)
include_directories(..)
add_executable(wsocket ${srcs})
target_link_libraries(wsocket)
