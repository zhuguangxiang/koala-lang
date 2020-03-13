#
# MIT License
# Copyright (c) 2018 James, https://github.com/zhuguangxiang
#

# add simple test with arguments
MACRO(TEST name)
  ADD_EXECUTABLE(${name} ${name}.c)
  TARGET_LINK_LIBRARIES(${name} ${ARGN})
  ADD_TEST(NAME ${name} COMMAND ${name})
ENDMACRO()
