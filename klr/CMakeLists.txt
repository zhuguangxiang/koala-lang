#
# This file is part of the koala-lang project, under the MIT License.
#
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#

add_compile_definitions(KLR_COMPILE)

set(IR_SRCS klr_type.c)

add_library(klr STATIC ${IR_SRCS})

target_link_libraries(klr util)
