#
# MIT License
# Copyright (c) 2018 James, https://github.com/zhuguangxiang
#

# symbol link
MACRO(INSTALL_SYMLINK target linkname)
  INSTALL(CODE
    "EXECUTE_PROCESS(
     COMMAND ${CMAKE_COMMAND} -E create_symlink ${target} ${linkname})")
ENDMACRO()
