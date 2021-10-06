cmake_minimum_required(VERSION 3.0)
set(CMAKE_VERBOSE_MAKEFILE ON)
project(sample)

file(GLOB srcs *.c ../*.c)
include_directories(..)
add_executable(sample ${srcs})
target_link_libraries(sample)
