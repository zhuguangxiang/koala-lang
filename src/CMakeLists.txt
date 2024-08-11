#
# This file is part of the koala project with MIT License.
# Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
#

include(${PROJECT_SOURCE_DIR}/cmake/bison.cmake)

run_flex(
    ${PROJECT_SOURCE_DIR}/src/parser/koala.l
    ${PROJECT_BINARY_DIR}/src/parser/koala_lex.c
    ${PROJECT_BINARY_DIR}/include/parser/koala_lex.h)

run_bison(
    ${PROJECT_SOURCE_DIR}/src/parser/koala.y
    ${PROJECT_BINARY_DIR}/src/parser/koala_yacc.c
    ${PROJECT_BINARY_DIR}/include/parser/koala_yacc.h)

set(PARSER_SRC
    ${PROJECT_BINARY_DIR}/src/parser/koala_yacc.c
    ${PROJECT_BINARY_DIR}/src/parser/koala_lex.c
    ${PROJECT_SOURCE_DIR}/src/parser/ast.c
    ${PROJECT_SOURCE_DIR}/src/parser/parser.c
    ${PROJECT_SOURCE_DIR}/src/parser/parser_expr.c
    ${PROJECT_SOURCE_DIR}/src/parser/symbol.c
    ${PROJECT_SOURCE_DIR}/src/parser/typedesc.c)

# add_library(parser STATIC ${PARSER_SRC})
# target_link_libraries(parser)
add_executable(koalac_out ${PARSER_SRC})
target_link_libraries(koalac_out koala)
set_target_properties(koalac_out PROPERTIES
    OUTPUT_NAME koalac)

set(KOALA_SOURCES
    common/atom.c
    common/log.c
    common/mm.c
    common/lldq.c
    common/vector.c
    common/hashmap.c
    common/buffer.c
    common/utf8.c
    object.c
    gc.c
    run.c
    eval.c
    moduleobject.c
    typeobject.c
    cfuncobject.c
    codeobject.c
    intobject.c
    strobject.c
    noneobject.c
    tupleobject.c
    listobject.c
    dictobject.c
    exception.c)

add_library(koala STATIC ${KOALA_SOURCES})
target_link_libraries(koala pthread m dl ffi)

add_executable(koala_out main.c)
target_link_libraries(koala_out koala)
set_target_properties(koala_out PROPERTIES
    OUTPUT_NAME koala)
