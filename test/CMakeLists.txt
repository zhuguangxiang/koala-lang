#
# MIT License
# Copyright (c) James, https://github.com/zhuguangxiang
#

include(${CMAKE_SOURCE_DIR}/cmake/test.cmake)

test(fast_string_cmp koala)
test(test_128bits)
test(test_bitset koala)
test(test_cfunc koala)
test(test_float_print)
# test(test_get_int_method koala)
test(test_hashmap koala)
# test(test_ir_inline)
# test(test_kwargs koala)
test(test_sds koala)
test(test_str_repeat)
# test(test_type_call koala)
# test(test_typeof koala)
test(test_vector koala)
