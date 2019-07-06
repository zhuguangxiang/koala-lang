# MIT License
#
# Copyright (c) 2018 James, https://github.com/zhuguangxiang
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(PROJECT_NAME "koala-lang")

PROJECT(${PROJECT_NAME})

SET(KOALA_VERSION_MAJOR 0)
SET(KOALA_VERSION_MINOR 9)
SET(KOALA_VERSION_PATCH 1)
SET(KOALA_VERSION_MAIN "${KOALA_VERSION_MAJOR}.${KOALA_VERSION_MINOR}")
SET(KOALA_VERSION "${KOALA_VERSION_MAIN}.${KOALA_VERSION_PATCH}")

CONFIGURE_FILE(include/version.h.in ${PROJECT_SOURCE_DIR}/include/version.h)

SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local/lib)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

IF(CMAKE_BUILD_TYPE STREQUAL Debug)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -g")
ELSE()
  IF(CMAKE_BUILD_TYPE STREQUAL NLOG)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -g -DNLOG")
  ELSE()
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -O2 -Wall")
  ENDIF()
ENDIF()

MESSAGE("-- Build:" ${CMAKE_BUILD_TYPE})
MESSAGE("-- Flags:" ${CMAKE_C_FLAGS})

OPTION(SKIP_TESTS "running test programs" OFF)

INCLUDE_DIRECTORIES(include)

ADD_SUBDIRECTORY(src)

IF(NOT SKIP_TESTS)
  ENABLE_TESTING()
  ADD_SUBDIRECTORY(test)
ENDIF()
