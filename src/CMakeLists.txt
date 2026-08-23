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
    parser/parser_generic.c
    parser/symbol.c
    parser/typespec.c
    parser/write_klc.c
    parser/read_klc.c
    parser/irgen.c
    parser/klr/opcode.c
    parser/klr/ir.c
    parser/klr/insn.c
    parser/klr/printer.c
    parser/klr/ssa.c
    parser/backend/isel.c
    parser/backend/lsra.c
    parser/backend/lower_operands.c
    parser/backend/cgen.c
    parser/opt/const_copy_prop.c
    parser/opt/dce.c
    parser/opt/cfg_bb_opt.c
    parser/opt/normalize.c
    parser/opt/opt.c
    parser/opt/pass.c
    parser/opt/phi_coalescing.c
    parser/opt/sccp.c
    )

add_library(parser STATIC ${PARSER_SRC})
target_link_libraries(parser koala_common m)

add_executable(koalac_out parser/koalac.c)
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

add_library(koala_common STATIC ${COMMON_SOURCES})
set_target_properties(koala_common PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(KOALA_SHARED_SOURCES
    objects/boolobj.c
    objects/floatobj.c
    objects/funcobj.c
    objects/fieldobj.c
    objects/intobj.c
    objects/modobj.c
    objects/noneobj.c
    objects/strobj.c
    objects/typeobj.c
    objects/tupleobj.c
    objects/rangeobj.c
    objects/listobj.c
    # objects/dictobj.c
    objects/bytesobj.c
    objects/bytebuf.c
    modules/builtin.c
    modules/format.c
    runtime/mod.c
    runtime/symtbl.c
    runtime/vm.c
    runtime/except.c
    )

add_library(koala SHARED ${KOALA_SHARED_SOURCES})
target_link_libraries(koala koala_common pthread m dl ffi)

set(KOALA_SOURCES
    runtime/eval.c
    args.c
    koala.c
    )
add_executable(koala_out ${KOALA_SOURCES})

target_link_libraries(koala_out koala)
set_target_properties(koala_out PROPERTIES
    OUTPUT_NAME koala)

add_executable(koala_dump ${PROJECT_SOURCE_DIR}/tools/klc-dump.c)
target_link_libraries(koala_dump parser)
set_target_properties(koala_dump PROPERTIES
    OUTPUT_NAME klc-dump)

install(TARGETS koala DESTINATION koala-${KOALA_VERSION_MAIN}/lib)
install(TARGETS koalac_out DESTINATION koala-${KOALA_VERSION_MAIN}/bin)
install(TARGETS koala_out DESTINATION koala-${KOALA_VERSION_MAIN}/bin)

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
    ../lib/koala/bin/koala
    ${CMAKE_INSTALL_PREFIX}/../bin/koala)

install_symlink(
    ../lib/koala/include
    ${CMAKE_INSTALL_PREFIX}/../include/koala)

install_symlink(
    ../lib/koala/bin/klc-dump
    ${CMAKE_INSTALL_PREFIX}/../bin/klc-dump)

install_symlink(
    ./koala/lib/libkoala.so
    ${CMAKE_INSTALL_PREFIX}/../lib/libkoala.so)
