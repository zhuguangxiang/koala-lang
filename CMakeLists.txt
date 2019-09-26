#
# MIT License
# Copyright (c) 2018 James, https://github.com/zhuguangxiang
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(PROJECT_NAME "koala-lang")

PROJECT(${PROJECT_NAME})

SET(KOALA_VERSION_MAJOR 0)
SET(KOALA_VERSION_MINOR 9)
SET(KOALA_VERSION_PATCH 1)
SET(KOALA_VERSION_MAIN "${KOALA_VERSION_MAJOR}.${KOALA_VERSION_MINOR}")
SET(KOALA_VERSION "${KOALA_VERSION_MAIN}.${KOALA_VERSION_PATCH}")

CONFIGURE_FILE(
  ${PROJECT_SOURCE_DIR}/include/version.h.in
  ${PROJECT_SOURCE_DIR}/include/version.h)

SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local/lib)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

IF(CMAKE_BUILD_TYPE STREQUAL Debug)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -g -DNDEBUG")
ELSE()
  IF(CMAKE_BUILD_TYPE STREQUAL Coverage)
  # lcov --rc lcov_branch_coverage=1 -c -d ./ -o app.info
  # genhtml --rc lcov_branch_coverage=1 app.info -o cc_result
    SET(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} -std=gnu11 -g -DNDEBUG -DNLog
        -fprofile-arcs -ftest-coverage")
  ELSE()
    IF(CMAKE_BUILD_TYPE STREQUAL NDebug)
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -g -DNDEBUG -DNLog")
    ELSE()
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -O2 -Wall -DNDEBUG -DNLog")
    ENDIF()
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
