#
# MIT License
# Copyright (c) 2018 James, https://github.com/zhuguangxiang
#

include(${CMAKE_SOURCE_DIR}/cmake/test.cmake)

test(test_float_print)
test(test_vector koala)
test(test_hashmap koala)
test(fast_string_cmp koala)
test(test_bitset koala)
test(test_cfunc koala)
test(test_tuple koala)
test(test_fib koala)
set_tests_properties(test_fib PROPERTIES LABELS no_debug_test)
test(test_type_call koala)
test(test_module koala)
test(test_kwargs koala)
test(test_typeof koala)
test(test_get_int_method koala)
test(test_ir parser)
test(test_remove_load_store parser)
test(test_constant_folding parser)
test(test_unused_block parser)
test(test_fib_klc koala)
test(test_klc koala)
set_tests_properties(test_fib_klc PROPERTIES LABELS no_debug_test)
