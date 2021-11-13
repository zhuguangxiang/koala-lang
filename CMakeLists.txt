#
# This file is part of the koala-lang project, under the MIT License.
#
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#

cmake_minimum_required(VERSION 2.8)

set(PROJECT_NAME "koala-lang")

set(ENABLE_TEST 1)
set(BUILD_TYPE DEBUG)

project(${PROJECT_NAME})

set(KOALA_VERSION_MAJOR 0)
set(KOALA_VERSION_MINOR 9)
set(KOALA_VERSION_PATCH 0)
set(KOALA_VERSION_MAIN "${KOALA_VERSION_MAJOR}.${KOALA_VERSION_MINOR}")
set(KOALA_VERSION "${KOALA_VERSION_MAIN}.${KOALA_VERSION_PATCH}")

configure_file(
    ${PROJECT_SOURCE_DIR}/version.h.in
    ${PROJECT_SOURCE_DIR}/version.h
)

set(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local/lib)

if(BUILD_TYPE STREQUAL DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -std=gnu11 -Wall -g -fvar-tracking")
endif()

# link_directories(${CMAKE_CURRENT_BINARY_DIR}/src)
include_directories(.)

add_subdirectory(util)
add_subdirectory(klvm)
# add_subdirectory(gc)
# add_subdirectory(v0.9)
add_subdirectory(parser)
add_subdirectory(main)
add_subdirectory(task)

if(ENABLE_TEST)
    enable_testing()
    add_subdirectory(test)
endif()
