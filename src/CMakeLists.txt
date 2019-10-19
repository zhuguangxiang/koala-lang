#
# MIT License
# Copyright (c) 2018 James, https://github.com/zhuguangxiang
#

INCLUDE(${PROJECT_SOURCE_DIR}/cmake/bison.cmake)

RUN_FLEX(
  ${PROJECT_SOURCE_DIR}/src/koala.l
  ${PROJECT_SOURCE_DIR}/src/koala_lex.c
  ${PROJECT_SOURCE_DIR}/include/koala_lex.h)

RUN_BISON(
  ${PROJECT_SOURCE_DIR}/src/koala.y
  ${PROJECT_SOURCE_DIR}/src/koala_yacc.c
  ${PROJECT_SOURCE_DIR}/include/koala_yacc.h)

SET(KOALA_OBJECTS_SOURCES
  object.c fieldobject.c methodobject.c classobject.c intobject.c
  floatobject.c stringobject.c tupleobject.c mapobject.c arrayobject.c
  moduleobject.c codeobject.c rangeobject.c iterobject.c closureobject.c
  enumobject.c)

SET(KOALA_BUILTIN_MODULES
  langmodule.c osmodule.c iomodule.c fmtmodule.c modules/assertmodule.c)

SET(KOALA_SOURCES
  utf8.c memory.c vector.c hashmap.c strbuf.c bytebuffer.c atom.c
  typedesc.c eval.c image.c opcode.c symbol.c ast.c parser.c
  koala_lex.c koala_yacc.c run.c compile.c interactive.c readline.c gc.c
  ${KOALA_OBJECTS_SOURCES} ${KOALA_BUILTIN_MODULES})

ADD_LIBRARY(koala STATIC ${KOALA_SOURCES})
TARGET_LINK_LIBRARIES(koala pthread m dl)
ADD_LIBRARY(koala_so SHARED ${KOALA_SOURCES})
TARGET_LINK_LIBRARIES(koala_so pthread m dl)
SET_TARGET_PROPERTIES(koala_so PROPERTIES
  VERSION ${KOALA_VERSION}
  SOVERSION ${KOALA_VERSION_MAJOR}
  OUTPUT_NAME koala)

ADD_EXECUTABLE(koala_out main.c)
TARGET_LINK_LIBRARIES(koala_out koala)
SET_TARGET_PROPERTIES(koala_out PROPERTIES
  OUTPUT_NAME koala)

INSTALL(TARGETS koala DESTINATION koala-${KOALA_VERSION_MAIN}/lib)
INSTALL(TARGETS koala_so DESTINATION koala-${KOALA_VERSION_MAIN}/lib)
INSTALL(TARGETS koala_out DESTINATION koala-${KOALA_VERSION_MAIN}/bin)

INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
  DESTINATION koala-${KOALA_VERSION_MAIN}/include
  FILES_MATCHING
  PATTERN "*.h"
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
