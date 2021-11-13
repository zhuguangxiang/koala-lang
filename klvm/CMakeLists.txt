#
# This file is part of the koala-lang project, under the MIT License.
#
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#

set(KLVM_SRCS
    klvm.c inst.c pass.c printer.c)

add_library(klvm STATIC ${KLVM_SRCS})
target_link_libraries(klvm util)
set_target_properties(klvm PROPERTIES
    OUTPUT_NAME koala-klvm)

add_library(klvm_so SHARED ${KLVM_SRCS})
target_link_libraries(klvm_so util_so)
set_target_properties(klvm_so PROPERTIES
    VERSION ${KOALA_VERSION}
    SOVERSION ${KOALA_VERSION_MAJOR}
    OUTPUT_NAME koala-klvm)
