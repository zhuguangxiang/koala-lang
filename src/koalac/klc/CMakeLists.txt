#
# This file is part of the koala-lang project, under the MIT License.
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#

set(KLM_SRC
    klm.c
    )

add_library(klm STATIC ${KLM_SRC})
target_link_libraries(klm common)
set_target_properties(klm PROPERTIES
    OUTPUT_NAME koala-klm)

add_library(klm_so SHARED ${KLM_SRC})
target_link_libraries(klm_so common_so)
set_target_properties(klm_so PROPERTIES
    VERSION ${KOALA_VERSION}
    SOVERSION ${KOALA_VERSION_MAJOR}
    OUTPUT_NAME koala-klm)

install(TARGETS klm DESTINATION koala-${KOALA_VERSION_MAIN}/lib)
install(TARGETS klm_so DESTINATION koala-${KOALA_VERSION_MAIN}/lib)
