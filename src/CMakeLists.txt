#
# This file is part of the koala-lang project, under the MIT License.
# Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
# OR OTHER DEALINGS IN THE SOFTWARE.
#

ADD_COMPILE_DEFINITIONS(COMPILE_IR)

INCLUDE(${PROJECT_SOURCE_DIR}/cmake/bison.cmake)

RUN_FLEX(
    ${PROJECT_SOURCE_DIR}/src/parser/koala.l
    ${PROJECT_SOURCE_DIR}/src/parser/koala_lex.c
    ${PROJECT_SOURCE_DIR}/include/koala_lex.h)

RUN_BISON(
    ${PROJECT_SOURCE_DIR}/src/parser/koala.y
    ${PROJECT_SOURCE_DIR}/src/parser/koala_yacc.c
    ${PROJECT_SOURCE_DIR}/include/koala_yacc.h)

SET(KOALA_SOURCES
    mm.c vector.c hashmap.c sbuf.c atom.c readline.c
    klvm/klvm_type.c
    parser/koala_lex.c parser/koala_yacc.c parser/interactive.c
    parser/parser.c)

ADD_LIBRARY(koala_so SHARED ${KOALA_SOURCES})
SET_TARGET_PROPERTIES(koala_so PROPERTIES
    OUTPUT_NAME koala)
