#
# This file is part of the koala-lang project, under the MIT License.
# Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
#

include(${PROJECT_SOURCE_DIR}/cmake/bison.cmake)

run_flex(
    ${PROJECT_SOURCE_DIR}/src/koalac/koala.l
    ${PROJECT_SOURCE_DIR}/src/koalac/koala_lex.c
    ${PROJECT_SOURCE_DIR}/src/koalac/koala_lex.h)

run_bison(
    ${PROJECT_SOURCE_DIR}/src/koalac/koala.y
    ${PROJECT_SOURCE_DIR}/src/koalac/koala_yacc.c
    ${PROJECT_SOURCE_DIR}/src/koalac/koala_yacc.h)

set(KOALAC_SOURCES
    koala_lex.c
    koala_yacc.c
    typedesc.c
    ast.c
    parser.c
    symbol.c
    main.c)

add_executable(koalac_out ${KOALAC_SOURCES})
target_link_libraries(koalac_out common)
set_target_properties(koalac_out PROPERTIES OUTPUT_NAME koalac)

INSTALL(TARGETS koalac_out DESTINATION koala-${KOALA_VERSION_MAIN}/bin)

INCLUDE(${PROJECT_SOURCE_DIR}/cmake/symlink.cmake)
INSTALL(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/../bin)
INSTALL_SYMLINK(
    ../lib/koala-${KOALA_VERSION_MAIN}/bin/koalac
    ${CMAKE_INSTALL_PREFIX}/../bin/koalac)
