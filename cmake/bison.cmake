#
# This file is part of the koala project, under the MIT License.
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
# OR OTHER DEALINGS IN THE SOFTWARE.
#

FIND_PACKAGE(FLEX REQUIRED)
FIND_PACKAGE(BISON REQUIRED)

# use bison to generate grammar's c and h file
MACRO(RUN_BISON input_y output_c output_h)
  ADD_CUSTOM_COMMAND(
    OUTPUT ${output_c}
           ${output_h}
    DEPENDS ${input_y}
    COMMAND ${BISON_EXECUTABLE}
            -dvt
            --output=${output_c}
            --defines=${output_h}
            ${input_y})
ENDMACRO()

# use flex to generate lex's c and h file
MACRO(RUN_FLEX input_l output_c output_h)
  ADD_CUSTOM_COMMAND(
    OUTPUT ${output_c}
           ${output_h}
    DEPENDS ${input_l}
    COMMAND ${FLEX_EXECUTABLE}
            --outfile=${output_c}
            --header-file=${output_h}
            ${input_l})
ENDMACRO()
