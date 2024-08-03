#
# This file is part of the koala project with MIT License.
# Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
#

include(${PROJECT_SOURCE_DIR}/cmake/bison.cmake)

set(KOALA_SOURCES
    common/log.c
    common/mm.c
    common/lldq.c
    common/vector.c
    common/hashmap.c
    object.c
    gc.c
    run.c
    eval.c
    moduleobject.c
    typeobject.c
    cfuncobject.c
    funcobject.c
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
