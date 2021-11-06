#
# This file is part of the koala-lang project, under the MIT License.
#
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#

add_executable(koala main.c interactive.c compile.c run.c)
target_link_libraries(koala util parser)

set_property(
    TARGET koala
    APPEND PROPERTY ADDITIONAL_CLEAN_FILES
        ${PROJECT_SOURCE_DIR}/version.h
        ${PROJECT_SOURCE_DIR}/parser/koala_yacc.output
    )

install(TARGETS koala DESTINATION koala-${KOALA_VERSION_MAIN}/bin)
install(FILES ${PROJECT_SOURCE_DIR}/version.h
    DESTINATION koala-${KOALA_VERSION_MAIN}/include)
