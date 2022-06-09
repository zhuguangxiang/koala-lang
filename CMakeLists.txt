#
# This file is part of the koala-lang project, under the MIT License.
# Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
#

cmake_minimum_required(VERSION 3.11)

set(PROJECT_NAME "koala-lang")

set(KOALA_VERSION_MAJOR 0)
set(KOALA_VERSION_MINOR 7)
set(KOALA_VERSION_PATCH 0)
set(KOALA_VERSION_MAIN "${KOALA_VERSION_MAJOR}.${KOALA_VERSION_MINOR}")
set(KOALA_VERSION "${KOALA_VERSION_MAIN}.${KOALA_VERSION_PATCH}")

project(${PROJECT_NAME} VERSION ${KOALA_VERSION})

configure_file(
    ${PROJECT_SOURCE_DIR}/src/version.h.in
    ${PROJECT_SOURCE_DIR}/src/version.h)

set(ENABLE_TEST 0)

set(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local/lib)
add_definitions(-DMEM_MAXSIZE=512*1024)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wextra -Wall -g")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wextra -Werror -Wall -O2")
endif()

include_directories(src)

add_subdirectory(src/common)
add_subdirectory(src/koalac)
#add_subdirectory(src/runtime)

if(ENABLE_TEST)
    enable_testing()
    add_subdirectory(test)
endif()
