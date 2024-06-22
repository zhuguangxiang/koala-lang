#
# This file is part of the koala project with MIT LIcense.
# Copyright (c) James <zhuguangxiang@gmail.com>
#

# symbol link
macro(INSTALL_SYMLINK target linkname)
    install(CODE
        "EXECUTE_PROCESS(
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${target} ${linkname})")
endmacro()
