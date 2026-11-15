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

add_executable(test_wsdeque test_wsdeque.c)
target_link_libraries(test_wsdeque koala_common pthread m)
add_test(NAME c_wsdeque COMMAND test_wsdeque)
set_property(GLOBAL APPEND PROPERTY KOALA_C_TEST_TARGETS test_wsdeque)

# wsdeque under ThreadSanitizer: the sources are compiled into the test so
# they are instrumented too. TSan cannot be combined with ASan.
if(NOT CMAKE_BUILD_TYPE STREQUAL DebugASAN)
    add_executable(test_wsdeque_tsan test_wsdeque.c
        ${PROJECT_SOURCE_DIR}/src/common/wsdeque.c
        ${PROJECT_SOURCE_DIR}/src/common/mm.c
        ${PROJECT_SOURCE_DIR}/src/common/log.c)
    target_compile_options(test_wsdeque_tsan PRIVATE
        -fsanitize=thread -fno-omit-frame-pointer -O1)
    target_link_options(test_wsdeque_tsan PRIVATE -fsanitize=thread)
    target_link_libraries(test_wsdeque_tsan pthread m)
    add_test(NAME c_wsdeque_tsan COMMAND test_wsdeque_tsan)
    set_tests_properties(c_wsdeque_tsan PROPERTIES
        ENVIRONMENT "TSAN_OPTIONS=halt_on_error=1")
    set_property(GLOBAL APPEND PROPERTY KOALA_C_TEST_TARGETS test_wsdeque_tsan)
endif()

add_executable(test_inline_cost test_inline_cost.c)
target_link_libraries(test_inline_cost parser)
add_test(NAME c_inline_cost COMMAND test_inline_cost)
set_property(GLOBAL APPEND PROPERTY KOALA_C_TEST_TARGETS test_inline_cost)

# re_core is compiled into the test directly: it lives in the koala/re package
# and has no build target of its own (packages are built by wombat).
add_executable(test_re_core test_re_core.c
    ${PROJECT_SOURCE_DIR}/libs/koala/re/native/re_core.c
    ${PROJECT_SOURCE_DIR}/libs/koala/re/native/re_unicode.c)
target_include_directories(test_re_core PRIVATE ${PROJECT_SOURCE_DIR}/libs/koala/re/native)
target_link_libraries(test_re_core koala_common m)
add_test(NAME c_re_core COMMAND test_re_core)
set_property(GLOBAL APPEND PROPERTY KOALA_C_TEST_TARGETS test_re_core)
