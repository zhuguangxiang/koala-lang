#
# This file is part of the koala-lang project, under the MIT License.
# Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
#

# add simple test with arguments
MACRO(TEST name)
    ADD_EXECUTABLE(${name} ${name}.c)
    TARGET_LINK_LIBRARIES(${name} ${ARGN})
    ADD_TEST(NAME ${name} COMMAND ${name})
ENDMACRO()
