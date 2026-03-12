#
# This file is part of the koala project with MIT License.
# Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
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
    parser/typespec.c
    parser/emit_ir.c
    parser/emit_code.c
    parser/write_klc.c
    parser/read_klc.c
    parser/klr/ir.c
    parser/klr/insn.c
    parser/klr/printer.c
    # parser/klr/insn_remap.c
    # parser/klr/linear_scan.c
    parser/klr/simple_reg_alloc.c
    parser/klr/pass.c
    parser/klr/passes/dot.c
    parser/klr/passes/remove_load_store.c
    # parser/klr/passes/constant_folding.c
    # parser/klr/passes/unused_var_insn.c
    parser/klr/passes/let_lit_prop.c
    parser/klr/passes/dce.c
    parser/klr/passes/cfg_bb_opt.c)

add_library(parser STATIC ${PARSER_SRC})
target_link_libraries(parser koala-common)

add_executable(koalac_out parser/main.c)
target_link_libraries(koalac_out parser)
set_target_properties(koalac_out PROPERTIES
    OUTPUT_NAME koalac)

set(COMMON_SOURCES
    common/sds.c
    common/atom.c
    common/log.c
    common/mm.c
    common/lldq.c
    common/vector.c
    common/hashmap.c
    common/buffer.c
    common/bitset.c
    common/utf8.c
    common/klc.c)

add_library(koala-common STATIC ${COMMON_SOURCES})

set(KOALA_SOURCES
    object.c
    typeobject.c
    anyobject.c
    noneobject.c
    boolobject.c
    # fieldobject.c
    cfuncobject.c
    codeobject.c
    intobject.c
    # floatobject.c
    stringobject.c
    tupleobject.c
    exception.c
    iterobject.c
    collectionobject.c
    numberobject.c
    moduleobject.c
    modules/builtin.c
    gc.c
    run.c
    eval.c
    read_klc.c
    # jit/jit.c
    )

add_library(koala STATIC ${KOALA_SOURCES})
target_link_libraries(koala koala-common pthread m dl ffi)

# add_executable(koala_out ${PROJECT_SOURCE_DIR}/src/main.c)
# target_link_libraries(koala_out koala)
# set_target_properties(koala_out PROPERTIES
#     OUTPUT_NAME koala)

add_executable(koala_dump ${PROJECT_SOURCE_DIR}/tools/koala_dump.c)
target_link_libraries(koala_dump parser)
set_target_properties(koala_dump PROPERTIES
    OUTPUT_NAME koala_dump)

install(TARGETS koalac_out DESTINATION koala-${KOALA_VERSION_MAIN}/bin)

install(DIRECTORY ${PROJECT_BINARY_DIR}/include/
    DESTINATION koala-${KOALA_VERSION_MAIN}/include
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "koala_lex.h" EXCLUDE
    PATTERN "koala_yacc.h" EXCLUDE)

install(TARGETS koala_dump DESTINATION koala-${KOALA_VERSION_MAIN}/bin)

include(${PROJECT_SOURCE_DIR}/cmake/symlink.cmake)

install_symlink(
    ./koala-${KOALA_VERSION_MAIN}
    ${CMAKE_INSTALL_PREFIX}/koala)

install_symlink(
    ../lib/koala/bin/koalac
    ${CMAKE_INSTALL_PREFIX}/../bin/koalac)

install_symlink(
    ../lib/koala/include
    ${CMAKE_INSTALL_PREFIX}/../include/koala)

install_symlink(
    ../lib/koala/bin/koala_dump
    ${CMAKE_INSTALL_PREFIX}/../bin/koala_dump)
