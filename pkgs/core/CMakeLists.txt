#
# This file is part of the koala-lang project, under the MIT License.
#
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#

set(CORE_SRCS
    core.c
    stringobject.c
    arrayobject.c
    reflect.c)

add_library(core STATIC ${CORE_SRCS})

target_link_libraries(core util gc)
