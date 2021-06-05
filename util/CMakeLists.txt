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
    bitvector.c
    atom.c
    readline.c)

add_library(util STATIC ${UTIL_SRCS})
