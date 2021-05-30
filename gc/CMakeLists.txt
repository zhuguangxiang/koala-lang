#
# This file is part of the koala-lang project, under the MIT License.
#
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#

set(GC_SRCS
    gc.c)

add_library(gc STATIC ${GC_SRCS})

target_link_libraries(gc util)
