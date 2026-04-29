#
# This file is part of the koala-lang project, under the MIT License.
# Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>
#

# Generate opcode_list_lowercase.h from opcode_list.h
# Pure CMake, no external tools.

set(OPCODE_LIST ${CMAKE_SOURCE_DIR}/include/common/opcode_list.h)
set(OPCODE_LOWERCASE ${CMAKE_BINARY_DIR}/include/opcode_list_lowercase.h)

file(STRINGS "${OPCODE_LIST}" OPCODE_LINES)

set(OPCODE_NAMES "")

foreach(line ${OPCODE_LINES})
    # Only match real opcode lines, skip comments
    if("${line}" MATCHES "^X\\(")
        # Capture OP_XXXX from X(OP_XXXX, ...)
        string(REGEX MATCH "X\\((OP_[A-Z0-9_]+)" op "${line}")
        if(op)
            list(APPEND OPCODE_NAMES "${CMAKE_MATCH_1}")
        endif()
    endif()
endforeach()

set(LOWERCASE_LINES "")

foreach(OP ${OPCODE_NAMES})
    # Strip OP_
    string(SUBSTRING "${OP}" 3 -1 NAME_ONLY)

    # Lowercase
    string(TOLOWER "${NAME_ONLY}" LOWER)

    # Dot-format rules
    if("${OP}" MATCHES "^OP_INT_" OR
       "${OP}" MATCHES "^OP_UINT_" OR
       "${OP}" MATCHES "^OP_FLOAT_" OR
       "${OP}" MATCHES "^OP_FLT_" OR
       "${OP}" MATCHES "^OP_REF_" OR
       "${OP}" MATCHES "^OP_CONST_")

        string(FIND "${LOWER}" "_" POS)
        if(NOT POS EQUAL -1)
            string(SUBSTRING "${LOWER}" 0 ${POS} PREFIX)
            math(EXPR START "${POS} + 1")
            string(SUBSTRING "${LOWER}" ${START} -1 SUFFIX)
            set(LOWER "${PREFIX}.${SUFFIX}")
        endif()
    endif()

    set(LOWERCASE_LINES "${LOWERCASE_LINES}\"${LOWER}\",\n")
endforeach()

file(WRITE "${OPCODE_LOWERCASE}" "${LOWERCASE_LINES}")
