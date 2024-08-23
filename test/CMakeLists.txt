#
# MIT License
# Copyright (c) 2018 James, https://github.com/zhuguangxiang
#

include(${CMAKE_SOURCE_DIR}/cmake/test.cmake)

test(test_vector koala)
test(test_hashmap koala)
test(test_cfunc koala)
test(test_tuple koala)
test(test_fib koala)
set_tests_properties(test_fib PROPERTIES LABELS no_debug_test)
test(test_type_call koala)
test(test_ir parser)
test(test_remove_load_store parser)
test(test_constant_folding parser)
