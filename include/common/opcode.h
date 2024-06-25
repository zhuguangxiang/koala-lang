/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_OPCODE_H_
#define _KOALA_OPCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */
typedef enum _OpCode {
/*+----------------------+--------------------------------------------------+*/
/*| name                 | arguments & comments                             |*/
/*+----------------------+--------------------------------------------------+*/
    OP_STOP,            /* stop the current koala state                      */

    OP_EXT_TAG,         /* extend operands byte code                         */

    OP_MOV,             /* A B              R(A) = R(B)                      */
    OP_PUSH,            /* A                R(top++) = R(A)                  */

    OP_CONST_LOAD,

    OP_CONST_I32_M1,
    OP_CONST_I32_0,
    OP_CONST_I32_1,
    OP_CONST_I32_2,
    OP_CONST_I32_3,
    OP_CONST_I32_4,
    OP_CONST_I32_5,
    OP_CONST_I32_IMM8,

    OP_CONST_I64_M1,
    OP_CONST_I64_0,
    OP_CONST_I64_1,
    OP_CONST_I64_2,
    OP_CONST_I64_3,
    OP_CONST_I64_4,
    OP_CONST_I64_5,
    OP_CONST_I64_IMM8,

    OP_INT_ADD,
    OP_INT_SUB,
    OP_INT_MUL,
    OP_INT_DIV,
    OP_INT_MOD,
    OP_INT_NEG,

    OP_INT_AND,
    OP_INT_OR,
    OP_INT_XOR,
    OP_INT_NOT,
    OP_INT_SHL,
    OP_INT_SHR,
    OP_INT_USHR,

    OP_INT_CMP,

    OP_INT_ADD_IMM8,
    OP_INT_SUB_IMM8,
    OP_INT_MUL_IMM8,
    OP_INT_DIV_IMM8,
    OP_INT_MOD_IMM8,

    OP_INT_AND_IMM8,
    OP_INT_OR_IMM8,
    OP_INT_XOR_IMM8,
    OP_INT_SHL_IMM8,
    OP_INT_SHR_IMM8,
    OP_INT_USHR_IMM8,

    OP_INT_CMP_IMM8,

    OP_FLOAT_ADD,
    OP_FLOAT_SUB,
    OP_FLOAT_MUL,
    OP_FLOAT_DIV,
    OP_FLOAT_MOD,
    OP_FLOAT_NEG,

    OP_FLOAT_CMPL,
    OP_FLOAT_CMPG,

    OP_AS_INT8,
    OP_AS_INT16,
    OP_AS_INT32,
    OP_AS_INT64,
    OP_AS_FLOAT32,
    OP_AS_FLOAT64,

    OP_AS,
    OP_IS,

    /* logic operations */
    OP_JUMP,
    OP_JUMP_IF_TRUE,
    OP_JUMP_IF_FALSE,

    /* call */
    OP_CALL,
    OP_CALL_KW,
    OP_CALL_METHOD,
    OP_CALL_METHOD_KW,
    OP_CALL_INTF,
    OP_CALL_INTF_KW,

    /* return */
    OP_RETURN,
    OP_RET_NONE,

    /* attribute(fields, getset, member, ...) */
    OP_ATTR_GET,
    OP_ATTR_SET,

    /* globals */
    OP_GLOBAL_GET,
    OP_GLOBAL_SET,

    /* ARRAY */
    OP_ARRAY_INT8_GET,
    OP_ARRAY_INT8_SET,
    OP_ARRAY_INT8_SLICE,

    OP_ARRAY_INT16_GET,
    OP_ARRAY_INT16_SET,
    OP_ARRAY_INT16_SLICE,

    OP_ARRAY_INT32_GET,
    OP_ARRAY_INT32_SET,
    OP_ARRAY_INT32_SLICE,

    OP_ARRAY_INT64_GET,
    OP_ARRAY_INT64_SET,
    OP_ARRAY_INT64_SLICE,

    OP_ARRAY_FLOAT32_GET,
    OP_ARRAY_FLOAT32_SET,
    OP_ARRAY_FLOAT32_SLICE,

    OP_ARRAY_FLOAT64_GET,
    OP_ARRAY_FLOAT64_SET,
    OP_ARRAY_FLOAT64_SLICE,

    OP_ARRAY_OBJECT_SET,
    OP_ARRAY_OBJECT_GET,
    OP_ARRAY_OBJECT_SLICE,

    /* MAP */
    OP_MAP_GET,
    OP_MAP_SET,

    /* number protocol */
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_NEG,

    OP_AND,
    OP_OR,
    OP_XOR,
    OP_NOT,
    OP_SHL,
    OP_SHR,
    OP_USHR,

    OP_CMP,

    /* sequence protocol (map, array, tuple, str, ...) */
    OP_SEQ_GET,             /* seq[index] */
    OP_SEQ_SET,             /* seq[index] = value */
    OP_SEQ_CONCAT,          /* seq1 + seq2 */
    OP_SEQ_SLICE,           /* seq[1...3] */
    OP_SEQ_SLICE_SET,       /* seq[1...3] = seq2 */

    /* iterator */
    OP_GET_ITER,
    OP_FOR_ITER,

    /* dynamic attribute */
    OP_DYN_ATTR_GET,
    OP_DNY_ATTR_SET,

    /* raise an error */
    OP_RAISE,

    OP_MAX,
} OpCode;
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPCODE_H_ */
