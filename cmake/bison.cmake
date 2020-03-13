#
# MIT License
# Copyright (c) 2018 James, https://github.com/zhuguangxiang
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
