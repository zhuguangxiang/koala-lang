#
# This file is part of the koala-lang project, under the MIT License.
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#

set(COMMON_SRC
    atom.c
    buffer.c
    hash.c
    hashmap.c
    mm.c
    vector.c
    )

add_library(common STATIC ${COMMON_SRC})
set_target_properties(common PROPERTIES
    OUTPUT_NAME koala-common)

add_library(common_so SHARED ${COMMON_SRC})
set_target_properties(common_so PROPERTIES
    VERSION ${KOALA_VERSION}
    SOVERSION ${KOALA_VERSION_MAJOR}
    OUTPUT_NAME koala-common)

install(TARGETS common DESTINATION koala-${KOALA_VERSION_MAIN}/lib)
install(TARGETS common_so DESTINATION koala-${KOALA_VERSION_MAIN}/lib)
