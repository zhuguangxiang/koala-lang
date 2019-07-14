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

INCLUDE(${PROJECT_SOURCE_DIR}/cmake/bison.cmake)

RUN_FLEX(
  ${PROJECT_SOURCE_DIR}/src/koala.l
  ${PROJECT_SOURCE_DIR}/src/koala_lex.c
  ${PROJECT_SOURCE_DIR}/include/koala_lex.h)

RUN_BISON(
  ${PROJECT_SOURCE_DIR}/src/koala.y
  ${PROJECT_SOURCE_DIR}/src/koala_yacc.c
  ${PROJECT_SOURCE_DIR}/include/koala_yacc.h)

SET(KOALA_BUILTIN_OBJECTS_SOURCES
  objects/moduleobject.c
  objects/stringobject.c
  objects/intobject.c
  objects/tupleobject.c
  objects/codeobject.c)

SET(KOALA_SOURCES
  memory.c vector.c hashmap.c strbuf.c atom.c bytebuffer.c path.c node.c
  object.c typedesc.c koala_lex.c koala_yacc.c
  koala.c interactive.c compile.c run.c
  ${KOALA_BUILTIN_OBJECTS_SOURCES})

ADD_LIBRARY(koala STATIC ${KOALA_SOURCES})
ADD_LIBRARY(koala_so SHARED ${KOALA_SOURCES})
TARGET_LINK_LIBRARIES(koala_so readline)
SET_TARGET_PROPERTIES(koala_so PROPERTIES
  VERSION ${KOALA_VERSION}
  SOVERSION ${KOALA_VERSION_MAJOR}
  OUTPUT_NAME koala)

ADD_EXECUTABLE(koala_out main.c ${KOALA_SOURCES})
TARGET_LINK_LIBRARIES(koala_out readline)
SET_TARGET_PROPERTIES(koala_out PROPERTIES
  OUTPUT_NAME koala)

INSTALL(TARGETS koala DESTINATION koala-${KOALA_VERSION_MAIN}/lib)
INSTALL(TARGETS koala_so DESTINATION koala-${KOALA_VERSION_MAIN}/lib)
INSTALL(TARGETS koala_out DESTINATION koala-${KOALA_VERSION_MAIN}/bin)

INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
  DESTINATION koala-${KOALA_VERSION_MAIN}/include
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "common.h" EXCLUDE
  PATTERN "koala_lex.h" EXCLUDE
  PATTERN "koala_yacc.h" EXCLUDE)

INCLUDE(${PROJECT_SOURCE_DIR}/cmake/symlink.cmake)
INSTALL_SYMLINK(
  ./koala-${KOALA_VERSION_MAIN}
  ${CMAKE_INSTALL_PREFIX}/koala)
INSTALL_SYMLINK(
  ../lib/koala-${KOALA_VERSION_MAIN}/bin/koala
  ${CMAKE_INSTALL_PREFIX}/../bin/koala)
INSTALL_SYMLINK(
  ../lib/koala-${KOALA_VERSION_MAIN}/include
  ${CMAKE_INSTALL_PREFIX}/../include/koala)
