/*
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

    OP_JMP_CMP_EQ,
    OP_JMP_CMP_NE,
    OP_JMP_CMP_LT,
    OP_JMP_CMP_GT,
    OP_JMP_CMP_LE,
    OP_JMP_CMP_GE,

    OP_JMP_EQZ,
    OP_JMP_NEZ,
    OP_JMP_LTZ,
    OP_JMP_GTZ,
    OP_JMP_LEZ,
    OP_JMP_GEZ,

    /* call */
    OP_CALL,            /* A B C  A = mod index, B = obj index, C = nargs */
    OP_CALL_METHOD,
    OP_CALL_DYNAMIC,

    /* return */
    OP_RETURN,
    OP_RETURN_NONE,

    /* globals */
    OP_GLOBAL_LOAD,
    OP_GLOBAL_STORE,

    /* fields */
    OP_FIELD_LOAD,
    OP_FIELD_STORE,

    /* array operations */
    OP_ARRAY_I8_LOAD,
    OP_ARRAY_I8_STORE,
    OP_ARRAY_I16_LOAD,
    OP_ARRAY_I16_STORE,
    OP_ARRAY_I32_LOAD,
    OP_ARRAY_I32_STORE,
    OP_ARRAY_I64_LOAD,
    OP_ARRAY_I64_STORE,
    OP_ARRAY_F32_LOAD,
    OP_ARRAY_F32_STORE,
    OP_ARRAY_F64_LOAD,
    OP_ARRAY_F64_STORE,

    /* map operations */
    OP_MAP_LOAD,            /* A B C            R(A) = B[C] */
    OP_MAP_STORE,            /* A B C            A[B] = C    */

    /* generic arithmetic operators */
    OP_BINARY_ADD,
    OP_BINARY_SUB,
    OP_BINARY_MUL,
    OP_BINARY_DIV,
    OP_BINARY_MOD,
    OP_UNARY_NEG,

    OP_BINARY_AND,
    OP_BINARY_OR,
    OP_BINARY_XOR,
    OP_UNARY_NOT,
    OP_BINARY_SHL,
    OP_BINARY_SHR,
    OP_BINARY_USHR,

    /* generic compare operation */
    OP_BINARY_CMP,

    /* generic subscript operations */
    OP_SUBSCR_LOAD,
    OP_SUBSCR_STORE,

    /* generic attribute operations */
    OP_ATTR_LOAD,
    OP_ATTR_STORE,

    /* generic iterator operations */
    OP_GET_ITER,
    OP_ITER_NEXT,

    /* raise an error */
    OP_RAISE,

    /* The below insns are only in IR */
    OP_IR_LOAD,
    OP_IR_STORE,
    OP_IR_PHI,

} OpCode;

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPCODE_H_ */
