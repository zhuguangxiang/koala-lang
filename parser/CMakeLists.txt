#
# This file is part of the koala-lang project, under the MIT License.
#
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#

include(${PROJECT_SOURCE_DIR}/cmake/bison.cmake)

run_flex(
    ${PROJECT_SOURCE_DIR}/parser/koala.l
    ${PROJECT_SOURCE_DIR}/parser/koala_lex.c
    ${PROJECT_SOURCE_DIR}/parser/koala_lex.h)

run_bison(
    ${PROJECT_SOURCE_DIR}/parser/koala.y
    ${PROJECT_SOURCE_DIR}/parser/koala_yacc.c
    ${PROJECT_SOURCE_DIR}/parser/koala_yacc.h)

set(PARSER_SOURCES
    koala_lex.c
    koala_yacc.c
    ast.c
    parser.c
    symbol.c)

add_library(parser STATIC ${PARSER_SOURCES})
target_link_libraries(parser util)
set_target_properties(parser PROPERTIES
    OUTPUT_NAME koala-parser)

add_library(parser_so SHARED ${PARSER_SOURCES})
target_link_libraries(parser_so util_so)
set_target_properties(parser_so PROPERTIES
    VERSION ${KOALA_VERSION}
    SOVERSION ${KOALA_VERSION_MAJOR}
    OUTPUT_NAME koala-parser)

install(TARGETS parser DESTINATION koala-${KOALA_VERSION_MAIN}/lib)
install(TARGETS parser_so DESTINATION koala-${KOALA_VERSION_MAIN}/lib)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/parser/
    DESTINATION koala-${KOALA_VERSION_MAIN}/include/parser
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "koala_lex.h" EXCLUDE
    PATTERN "koala_yacc.h" EXCLUDE)
