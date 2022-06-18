#
# This file is part of the koala-lang project, under the MIT License.
# Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
#

# add simple test with arguments
macro(TEST name)
    add_executable(${name} ${name}.c)
    target_link_libraries(${name} ${ARGN})
    add_test(NAME ${name} COMMAND ${name})
endmacro()
