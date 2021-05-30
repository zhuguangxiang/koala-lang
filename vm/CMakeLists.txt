#
# This file is part of the koala-lang project, under the MIT License.
#
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#

set(VM_SRCS object.c)

add_library(vm STATIC ${VM_SRCS})

target_link_libraries(vm util)
