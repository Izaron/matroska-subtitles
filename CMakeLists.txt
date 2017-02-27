cmake_minimum_required(VERSION 3.6)
project(ebml_parser)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES matroska.c matroska.h)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -Wall -g -std=gnu99 -Wno-write-strings -D_FILE_OFFSET_BITS=64")
add_executable(ebml_parser ${SOURCE_FILES})