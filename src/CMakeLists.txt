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
    parser/ast.c
    parser/parser.c
    parser/parser_expr.c
    parser/symbol.c
    parser/typedesc.c
    parser/ir.c
    parser/insn.c
    parser/printer.c
    parser/insn_remap.c
    parser/pass.c
    parser/passes/dot.c
    parser/passes/remove_load_store.c
    parser/passes/constant_folding.c
    parser/passes/unused_var_insn.c
    parser/passes/basic_block.c)

add_library(parser STATIC ${PARSER_SRC})
target_link_libraries(parser koala)

add_executable(koalac_out parser/main.c)
target_link_libraries(koalac_out parser)
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

add_executable(koala_out ${PROJECT_SOURCE_DIR}/src/main.c)
target_link_libraries(koala_out koala)
set_target_properties(koala_out PROPERTIES
    OUTPUT_NAME koala)
