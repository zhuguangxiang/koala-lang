#
# This file is part of the koala-lang project, under the MIT License.
#
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#

# symbol link
MACRO(INSTALL_SYMLINK target linkname)
    INSTALL(CODE
        "EXECUTE_PROCESS(
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${target} ${linkname})")
ENDMACRO()
