#
# This file is part of the koala-lang project, under the MIT License.
#
# Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
#

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# use bison to generate grammar's c and h file
macro(run_bison input_y output_c output_h)
    add_custom_command(
        OUTPUT  ${output_c}
                ${output_h}
        DEPENDS ${input_y}
        COMMAND ${BISON_EXECUTABLE}
            -dvt
            --output=${output_c}
            --defines=${output_h}
            ${input_y})
endmacro()

# use flex to generate lex's c and h file
macro(run_flex input_l output_c output_h)
    add_custom_command(
        OUTPUT  ${output_c}
                ${output_h}
        DEPENDS ${input_l}
        COMMAND ${FLEX_EXECUTABLE}
            --outfile=${output_c}
            --header-file=${output_h}
            ${input_l})
endmacro()
