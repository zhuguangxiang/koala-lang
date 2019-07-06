# MIT License
#
# Copyright (c) 2018 James, https://github.com/zhuguangxiang
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

SET(COMMON_SOURCES
  memory.c vector.c
  )
ADD_LIBRARY(common STATIC ${COMMON_SOURCES})

INCLUDE(${PROJECT_SOURCE_DIR}/cmake/bison.cmake)

RUN_FLEX(
  ${PROJECT_SOURCE_DIR}/src/koala.lex
  ${PROJECT_SOURCE_DIR}/src/koala_lex.c
  ${PROJECT_SOURCE_DIR}/include/koala_lex.h
  )

RUN_BISON(
  ${PROJECT_SOURCE_DIR}/src/koala.y
  ${PROJECT_SOURCE_DIR}/src/koala_yacc.c
  ${PROJECT_SOURCE_DIR}/include/koala_yacc.h
  )

SET(KOALA_SOURCES
  koala_lex.c koala_yacc.c interactive.c main.c
  )
ADD_EXECUTABLE(koala ${KOALA_SOURCES})
TARGET_LINK_LIBRARIES(koala common readline)
