#
# This file is part of the koala-lang project, under the MIT License.
#
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#

include(${CMAKE_SOURCE_DIR}/cmake/test.cmake)

test(test_list)
test(test_vector util)
test(test_hashmap util)
test(test_binheap util)
test(test_buffer util)
test(test_bitvector util)
test(test_atom util)

# test(test_gc gc)

# test(test_mixin core)
# test(test_objects core)
# test(test_arrayobj core)
# test(test_mapobj core)
# test(test_desc core)

# test(test_interactive util)
# test(test_readline util)

test(test_klvm klvm)

## test(test_liveness koala)
# test(test_vm core)
test(test_task task)
test(test_timer task)
test(test_task_done task)
