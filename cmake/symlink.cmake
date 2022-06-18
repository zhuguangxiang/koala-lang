#
# This file is part of the koala-lang project, under the MIT License.
# Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
#

# symbol link
macro(INSTALL_SYMLINK target linkname)
    install(CODE
        "EXECUTE_PROCESS(
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${target} ${linkname})")
endmacro()
