cmake_minimum_required(VERSION 3.6)
project(ebml_parser)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES matroska.c matroska.h)
add_executable(ebml_parser ${SOURCE_FILES})