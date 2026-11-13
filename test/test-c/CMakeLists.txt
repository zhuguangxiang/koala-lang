#
# This file is part of the koala project with MIT License.
# Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
#
# C unit tests, registered in ctest as `c_<module>`.
#
# Each test lives in its own file, `test_<module>.c`, next to this CMakeLists,
# and includes the module header from include/common. The module itself already
# lives in `koala_common`, and an archive member is only pulled in when a symbol
# is still missing, so linking it here gives the test its dependencies (mm,
# log, ...) without duplicating the source under test.

function(common_utest name)
    add_executable(test_${name} test_${name}.c)
    target_link_libraries(test_${name} koala_common m)

    add_test(NAME c_${name} COMMAND test_${name})

    set_property(GLOBAL APPEND PROPERTY KOALA_C_TEST_TARGETS test_${name})
endfunction()

common_utest(ring)
common_utest(utf8)
common_utest(dominance)
common_utest(gvn)

# re_core is compiled into the test directly: it lives in the koala/re package
# and has no build target of its own (packages are built by wombat).
add_executable(test_re_core test_re_core.c
    ${PROJECT_SOURCE_DIR}/libs/koala/re/native/re_core.c
    ${PROJECT_SOURCE_DIR}/libs/koala/re/native/re_unicode.c)
target_include_directories(test_re_core PRIVATE ${PROJECT_SOURCE_DIR}/libs/koala/re/native)
target_link_libraries(test_re_core koala_common m)
add_test(NAME c_re_core COMMAND test_re_core)
set_property(GLOBAL APPEND PROPERTY KOALA_C_TEST_TARGETS test_re_core)
