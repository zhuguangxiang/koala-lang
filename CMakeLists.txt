#
# This file is part of the koala-lang project, under the MIT License.
#
# Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>
#

cmake_minimum_required(VERSION 2.8)

set(PROJECT_NAME "koala-lang")

set(ENABLE_TEST 1)
set(DEBUG_TYPE DEBUG)

project(${PROJECT_NAME})

set(KOALA_VERSION_MAJOR 0)
set(KOALA_VERSION_MINOR 5)
set(KOALA_VERSION_PATCH 0)
set(KOALA_VERSION_MAIN "${KOALA_VERSION_MAJOR}.${KOALA_VERSION_MINOR}")
set(KOALA_VERSION "${KOALA_VERSION_MAIN}.${KOALA_VERSION_PATCH}")

configure_file(
  ${PROJECT_SOURCE_DIR}/parser/version.h.in
  ${PROJECT_SOURCE_DIR}/parser/version.h
)

if(DEBUG_TYPE STREQUAL DEBUG)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -std=gnu11 -Wall -g -fvar-tracking")
endif()

# link_directories(${CMAKE_CURRENT_BINARY_DIR}/src)
include_directories(. pkgs)

add_subdirectory(util)
add_subdirectory(klr)
add_subdirectory(gc)
#add_subdirectory(pkgs/core)
add_subdirectory(vm)

if(ENABLE_TEST)
  enable_testing()
  add_subdirectory(test)
endif()
