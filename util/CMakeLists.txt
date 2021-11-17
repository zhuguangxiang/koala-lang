#
# This file is part of the koala-lang project, under the MIT License.
#
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#

set(UTIL_SRCS
    mm.c
    vector.c
    hash.c
    hashmap.c
    binheap.c
    buffer.c
    bitset.c
    atom.c
    readline.c
    typedesc.c)

add_library(util STATIC ${UTIL_SRCS})
set_target_properties(util PROPERTIES
    OUTPUT_NAME koala-util)

add_library(util_so SHARED ${UTIL_SRCS})
set_target_properties(util_so PROPERTIES
    VERSION ${KOALA_VERSION}
    SOVERSION ${KOALA_VERSION_MAJOR}
    OUTPUT_NAME koala-util)

install(TARGETS util DESTINATION koala-${KOALA_VERSION_MAIN}/lib)
install(TARGETS util_so DESTINATION koala-${KOALA_VERSION_MAIN}/lib)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/util/
    DESTINATION koala-${KOALA_VERSION_MAIN}/include/util
    FILES_MATCHING
    PATTERN "*.h")
