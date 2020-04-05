#
# MIT License
# Copyright (c) 2018 James, https://github.com/zhuguangxiang
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(PROJECT_NAME "koala-lang")

PROJECT(${PROJECT_NAME})

IF (${APPLE})
SET(CMAKE_MACOSX_RPATH 1)
MESSAGE("MacOS")
ENDIF()

SET(KOALA_VERSION_MAJOR 0)
SET(KOALA_VERSION_MINOR 8)
SET(KOALA_VERSION_PATCH 5)
SET(KOALA_VERSION_MAIN "${KOALA_VERSION_MAJOR}.${KOALA_VERSION_MINOR}")
SET(KOALA_VERSION "${KOALA_VERSION_MAIN}.${KOALA_VERSION_PATCH}")

CONFIGURE_FILE(
  ${PROJECT_SOURCE_DIR}/include/version.h.in
  ${PROJECT_SOURCE_DIR}/include/version.h)

SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local/lib)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE NDebug)
ENDIF()

IF(CMAKE_BUILD_TYPE STREQUAL Debug)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -g -fvar-tracking -DNDEBUG")
ELSE()
  IF(CMAKE_BUILD_TYPE STREQUAL Coverage)
  # lcov --rc lcov_branch_coverage=1 -c -d ./ -o app.info
  # genhtml --rc lcov_branch_coverage=1 app.info -o cc_result
    SET(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} -std=gnu11 -g -fvar-tracking -DNDEBUG -DNLog \
        -fprofile-arcs -ftest-coverage")
  ELSE()
    IF(CMAKE_BUILD_TYPE STREQUAL NDebug)
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -g -fvar-tracking -DNDEBUG -DNLog")
    ELSE()
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -O2 -Wall \
        -fomit-frame-pointer -foptimize-strlen -ffast-math -DNDEBUG -DNLog")
    ENDIF()
  ENDIF()
ENDIF()

MESSAGE("-- Build:" ${CMAKE_BUILD_TYPE})
MESSAGE("-- Flags:" ${CMAKE_C_FLAGS})

SET(CMAKE_INSTALL_MESSAGE LAZY)

OPTION(SKIP_TESTS "running test programs" OFF)

INCLUDE_DIRECTORIES(include)

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(pkgs/jit)

SET(SKIP_TESTS 1)

IF(NOT SKIP_TESTS)
  ENABLE_TESTING()
  ADD_SUBDIRECTORY(test)
ENDIF()

INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/pkgs/
  DESTINATION koala-${KOALA_VERSION_MAIN}/pkgs
  FILES_MATCHING
  PATTERN "*.kl"
  PATTERN "*.klc")