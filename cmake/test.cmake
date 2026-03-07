#
# This file is part of the koala project with MIT LIcense.
# Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>
#

# add simple test with arguments
macro(TEST name)
    add_executable(${name} ${name}.c)
    target_link_libraries(${name} ${ARGN})
    add_test(NAME ${name} COMMAND ${name})
endmacro()
