/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2022-2032 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_OPCODE_H_
#define _KOALA_OPCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* byte codes of Koala VM */
typedef enum {
    // clang-format off

    OP_HALT,                /* halt VM */

    /* (1) number constants */

    OP_I32_CONST,           /* push i32 value */
    OP_I64_CONST,           /* push i64 value */
    OP_F32_CONST,           /* push f32 value */
    OP_F64_CONST,           /* push f64 value */

    /* (2) arithmetic operations */

    OP_I32_ADD,             /* pop two i32, add and push back */
    OP_I32_SUB,             /* pop two i32, sub and push back */
    OP_I32_MUL,             /* pop two i32, mul and push back */
    OP_I32_DIV,             /* pop two i32, div and push back */
    OP_I32_REM,             /* pop two i32, rem and push back */

    OP_I64_ADD,             /* pop two i64, add and push back */
    OP_I64_SUB,             /* pop two i64, sub and push back */
    OP_I64_MUL,             /* pop two i64, mul and push back */
    OP_I64_DIV,             /* pop two i64, div and push back */
    OP_I64_REM,             /* pop two i64, rem and push back */

    OP_F32_ADD,             /* pop two f32, add and push back */
    OP_F32_SUB,             /* pop two f32, sub and push back */
    OP_F32_MUL,             /* pop two f32, mul and push back */
    OP_F32_DIV,             /* pop two f32, div and push back */

    OP_F64_ADD,             /* pop two f64, add and push back */
    OP_F64_SUB,             /* pop two f64, sub and push back */
    OP_F64_MUL,             /* pop two f64, mul and push back */
    OP_F64_DIV,             /* pop two f64, div and push back */

    /* (3) bitwise operations */

    OP_I32_AND,             /* pop two i32, bit_and and push back */
    OP_I32_OR,              /* pop two i32, bit_or  and push back */
    OP_I32_XOR,             /* pop two i32, bit_xor and push back */
    OP_I32_NOT,             /* pop two i32, bit_not and push back */
    OP_I32_SHL,             /* pop two i32, bit_shl and push back */
    OP_I32_SHR,             /* pop two i32, bit_shr and push back */
    OP_I32_USHR,            /* pop two i32, bit_ushr and push back */

    OP_I64_AND,             /* pop two i64, bit_and and push back */
    OP_I64_OR,              /* pop two i64, bit_or  and push back */
    OP_I64_XOR,             /* pop two i64, bit_xor and push back */
    OP_I64_NOT,             /* pop two i64, bit_not and push back */
    OP_I64_SHL,             /* pop two i64, bit_shl and push back */
    OP_I64_SHR,             /* pop two i64, bit_shr and push back */
    OP_I64_USHR,            /* pop two i64, bit_ushr and push back */

    /* (4) compare operations */

    OP_I32_EQ,              /* pop two i32, equal and push back */
    OP_I32_NE,              /* pop two i32, not equal and push back */
    OP_I32_GT,              /* pop two i32, greater than and push back */
    OP_I32_GE,              /* pop two i32, greater equal and push back */
    OP_I32_LT,              /* pop two i32, less than and push back */
    OP_I32_LE,              /* pop two i32, less equal and push back */

    OP_I64_EQ,              /* pop two i64, equal and push back */
    OP_I64_NE,              /* pop two i64, not equal and push back */
    OP_I64_GT,              /* pop two i64, greater than and push back */
    OP_I64_GE,              /* pop two i64, greater equal and push back */
    OP_I64_LT,              /* pop two i64, less than and push back */
    OP_I64_LE,              /* pop two i64, less equal and push back */

    OP_F32_EQ,              /* pop two f32, equal and push back */
    OP_F32_NE,              /* pop two f32, not equal and push back */
    OP_F32_GT,              /* pop two f32, greater than and push back */
    OP_F32_GE,              /* pop two f32, greater equal and push back */
    OP_F32_LT,              /* pop two f32, less than and push back */
    OP_F32_LE,              /* pop two f32, less equal and push back */

    OP_F64_EQ,              /* pop two f64, equal and push back */
    OP_F64_NE,              /* pop two f64, not equal and push back */
    OP_F64_GT,              /* pop two f64, greater than and push back */
    OP_F64_GE,              /* pop two f64, greater equal and push back */
    OP_F64_LT,              /* pop two f64, less than and push back */
    OP_F64_LE,              /* pop two f64, less equal and push back */

    /* (5) bool operations */
    OP_BOOL_AND,            /* pop two bool, bool_and and push back */
    OP_BOOL_OR,             /* pop two bool, bool_or and push back */
    OP_BOOL_NOT,            /* pop two bool, bool_not and push back */

    /* (6) variable access */
    OP_LOCAL_GET,           /* push local variable value into stack */
    OP_LOCAL_SET,           /* pop value and set to local variable */
    OP_LOCAL_REF,           /* push local variable address into stack */

    OP_GLOBAL_GET,
    OP_GLOBAL_SET,
    OP_GLOBAL_REF,

    /* (7) control flow */
    OP_JMP,
    OP_JMP_TRUE,
    OP_JMP_FALSE,

    OP_CALL,
    OP_CALL_INDIRECT,
    OP_RETURN,
    OP_RETURN_VALUE,

    /* (8) memory operations */
    OP_I32_LOAD,
    OP_I32_STORE,
    OP_I64_LOAD

    // clang-format on
} KlOpCode;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPCODE_H_ */
