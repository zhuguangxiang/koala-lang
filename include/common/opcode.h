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
/*+-------------------------+--------------------------------------------------+*/
/*| name                    | arguments & comments                             |*/
/*+-------------------------+--------------------------------------------------+*/
    OP_NOP,                 /*                  NOP                             */
    OP_MOV,                 /* A B              R(A) = R(B)                     */
    OP_PUSH,                /* A                STK(top++) = R(A)               */
    OP_POP,                 /* A                R(A) = STK(--top)               */
    OP_PUSH_NONE,           /*                  STK(top++) = None               */
    OP_PUSH_IMM8,

    OP_CONST_LOAD,          /* A K16            R(A) = CP(K16)                  */
    OP_CONST_NONE,          /* A                R(A) = None                     */
    OP_CONST_INT_M1,        /* A                R(A) = -1                       */
    OP_CONST_INT_0,         /* A                R(A) = 0                        */
    OP_CONST_INT_1,
    OP_CONST_INT_2,
    OP_CONST_INT_3,
    OP_CONST_INT_4,
    OP_CONST_INT_5,
    OP_CONST_FLOAT_0,
    OP_CONST_FLOAT_1,
    OP_CONST_FLOAT_2,
    OP_CONST_FLOAT_3,

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

    OP_INT_AS_INT8,
    OP_INT_AS_INT16,
    OP_INT_AS_INT32,
    OP_INT_AS_INT64,
    OP_INT_AS_FLOAT32,
    OP_INT_AS_FLOAT64,

    OP_FLOAT_AS_INT8,
    OP_FLOAT_AS_INT16,
    OP_FLOAT_AS_INT32,
    OP_FLOAT_AS_INT64,
    OP_FLOAT_AS_FLOAT32,
    OP_FLOAT_AS_FLOAT64,

    /* number operations */
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

    /* compare operation */
    OP_CMP,

    /* cast operation */
    OP_AS,
    OP_IS,

    /* jump/logic operations */
    OP_JMP,
    OP_JMP_NONE,
    OP_JMP_NOT_NONE,

    OP_JMP_INT_CMP_EQ,
    OP_JMP_INT_CMP_NE,
    OP_JMP_INT_CMP_LT,
    OP_JMP_INT_CMP_GT,
    OP_JMP_INT_CMP_LE,
    OP_JMP_INT_CMP_GE,

    OP_JMP_INT_CMP_EQ_IMM8,
    OP_JMP_INT_CMP_NE_IMM8,
    OP_JMP_INT_CMP_LT_IMM8,
    OP_JMP_INT_CMP_GT_IMM8,
    OP_JMP_INT_CMP_LE_IMM8,
    OP_JMP_INT_CMP_GE_IMM8,

    OP_JMP_EQ,
    OP_JMP_NE,
    OP_JMP_LT,
    OP_JMP_GT,
    OP_JMP_LE,
    OP_JMP_GE,

    /* call */
    OP_CALL,            /* A B C  A = mod index, B = obj index, C = nargs */
    OP_CALL_METHOD,
    OP_CALL_DYNAMIC,

    /* return */
    OP_RETURN,
    OP_RETURN_NONE,

    /* globals */
    OP_GLOBAL_GET,
    OP_GLOBAL_SET,

    /* attribute(fields, getset, ...) */
    OP_ATTR_GET,
    OP_ATTR_SET,

    /* map operations */
    OP_MAP_GET,            /* A B C            R(A) = B[C] */
    OP_MAP_SET,            /* A B C            A[B] = C    */

    /* iterator operations */
    OP_GET_ITER,
    OP_FOR_ITER,

    /* dynamic attribute */
    OP_DYN_ATTR_GET,
    OP_DYN_ATTR_SET,

    /* raise an error */
    OP_RAISE,

    OP_MAX,
} OpCode;
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPCODE_H_ */
