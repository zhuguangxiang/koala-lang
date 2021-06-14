#
# This file is part of the koala-lang project, under the MIT License.
#
# Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>
#

include(${CMAKE_SOURCE_DIR}/cmake/test.cmake)

test(test_list)
test(test_vector util)
test(test_hashmap util)
test(test_binheap util)
test(test_buffer util)
test(test_bitvector util)
test(test_atom util)
test(test_klr_type klr)
test(test_mixin core)
test(test_gc gc)
test(test_objects core)
test(test_arrayobj core)
test(test_mapobj core)

# test(test_interactive util)
# test(test_readline util)

## test(test_klvm koala)
## test(test_liveness koala)
test(test_vm vm util)
test(test_vm_fib vm util)
