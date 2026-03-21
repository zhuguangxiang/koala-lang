#
# This file is part of the koala-lang project, under the MIT License.
# Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>
#

# Generate opcode_list_lowercase.h from opcode_list.h
# Pure CMake, no external tools.

set(OPCODE_LIST ${CMAKE_SOURCE_DIR}/include/common/opcode_list.h)
set(OPCODE_LOWERCASE ${CMAKE_BINARY_DIR}/include/opcode_list_lowercase.h)

# Read the opcode list
file(READ ${OPCODE_LIST} OPCODE_CONTENTS)

# Extract OP_XXXX names
string(REGEX MATCHALL "OP_[A-Z0-9_]+" OPCODE_NAMES ${OPCODE_CONTENTS})

# Convert to lowercase and strip OP_
set(LOWERCASE_LINES "")

foreach(OP ${OPCODE_NAMES})
    # Strip OP_
    string(SUBSTRING ${OP} 3 -1 NAME_ONLY)

    # Lowercase: INT_SUB_IMM → int_sub_imm
    string(TOLOWER ${NAME_ONLY} LOWER)

    # Special rule: only OP_INT_* → int.xxx
    if(${OP} MATCHES "^OP_INT_" OR
       ${OP} MATCHES "^OP_UINT_" OR
       ${OP} MATCHES "^OP_FLOAT_" OR
       ${OP} MATCHES "^OP_FLT_" OR
       ${OP} MATCHES "^OP_CONST_")
        # Find first underscore
        string(FIND "${LOWER}" "_" POS)

        if(NOT POS EQUAL -1)
            # prefix = int
            string(SUBSTRING "${LOWER}" 0 ${POS} PREFIX)

            # suffix = sub_imm
            math(EXPR START "${POS} + 1")
            string(SUBSTRING "${LOWER}" ${START} -1 SUFFIX)

            # final = int.sub_imm
            set(LOWER "${PREFIX}.${SUFFIX}")
        endif()
    endif()

    # Append to output
    set(LOWERCASE_LINES "${LOWERCASE_LINES}\"${LOWER}\",\n")
endforeach()

# Write output
file(WRITE ${OPCODE_LOWERCASE} ${LOWERCASE_LINES})
