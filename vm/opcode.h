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

    /* (1) Int8/Int16 operations */

    OP_I8_PUSH,            /* push i8(-128,127) */
    OP_I16_PUSH,           /* push i8(-32768,32767) */

    /* (2) int32 operations */

    OP_I32_CONST_M1,        /* push (i32)-1 */
    OP_I32_CONST_0,         /* push (i32)0  */
    OP_I32_CONST_1,         /* push (i32)1  */
    OP_I32_CONST_2,         /* push (i32)2  */
    OP_I32_CONST_3,         /* push (i32)3  */
    OP_I32_CONST_4,         /* push (i32)4  */
    OP_I32_CONST_5,         /* push (i32)5  */

    OP_I32_ADD,             /* add two i32 */
    OP_I32_SUB,             /* sub two i32 */
    OP_I32_MUL,             /* mul two i32 */
    OP_I32_DIV,             /* div two i32 */
    OP_I32_REM,             /* rem two i32 */
    OP_I32_NEG,             /* negate i32  */

    OP_I32_AND,             /* bitwise and i32 */
    OP_I32_OR,              /* bitwise or i32  */
    OP_I32_XOR,             /* bitwise xor i32 */
    OP_I32_NOT,             /* bitwise not i32(-1 xor n) */
    OP_I32_SHL,             /* bitwise shift left i32 */
    OP_I32_SHR,             /* bitwise shift right i32 */
    OP_I32_USHR,            /* bitwise logic shift right i32 */

    OP_I32_EQ,              /* two i32 equal */
    OP_I32_NE,              /* two i32 not equal */
    OP_I32_GT,              /* two i32 greater than */
    OP_I32_GE,              /* two i32 greater equal */
    OP_I32_LT,              /* two i32 less than */
    OP_I32_LE,              /* two i32 lses equal */

    OP_I32_RETURN,          /* return i32 value */

    /* (3) int64 operations */

    OP_I64_CONST_0,         /* push (i64)0  */
    OP_I64_CONST_1,         /* push (i64)1  */

    OP_I64_ADD,             /* add two i64 */
    OP_I64_SUB,             /* sub two i64 */
    OP_I64_MUL,             /* mul two i64 */
    OP_I64_DIV,             /* div two i64 */
    OP_I64_REM,             /* rem two i64 */
    OP_I64_NEG,             /* negate i64  */

    OP_I64_AND,             /* bitwise and i64 */
    OP_I64_OR,              /* bitwise or i64  */
    OP_I64_XOR,             /* bitwise xor i64 */
    OP_I64_NOT,             /* bitwise not i64(-1 xor n) */
    OP_I64_SHL,             /* bitwise shift left i64 */
    OP_I64_SHR,             /* bitwise shift right i64 */
    OP_I64_USHR,            /* bitwise logic shift right i64 */

    OP_I64_EQ,              /* two i64 equal */
    OP_I64_NE,              /* two i64 not equal */
    OP_I64_GT,              /* two i64 greater than */
    OP_I64_GE,              /* two i64 greater equal */
    OP_I64_LT,              /* two i64 less than */
    OP_I64_LE,              /* two i64 lses equal */

    OP_I64_RETURN,          /* return i64 value */

    /* (4) float32/float64 operations */

    OP_F32_CONST_0,         /* push (f32)0.0  */
    OP_F32_CONST_1,         /* push (f32)1.0  */
    OP_F32_CONST_2,         /* push (f32)2.0  */

    OP_F32_ADD,             /* add two f32 */
    OP_F32_SUB,             /* sub two f32 */
    OP_F32_MUL,             /* mul two f32 */
    OP_F32_DIV,             /* div two f32 */
    OP_F32_NEG,             /* negate f32  */

    OP_F32_EQ,              /* two f32 equal */
    OP_F32_NE,              /* two f32 not equal */
    OP_F32_GT,              /* two f32 greater than */
    OP_F32_GE,              /* two f32 greater equal */
    OP_F32_LT,              /* two f32 less than */
    OP_F32_LE,              /* two f32 less equal */

    OP_F32_RETURN,          /* return f32 value */

    OP_F64_CONST_0,         /* push (f64)0.0  */
    OP_F64_CONST_1,         /* push (f64)1.0  */

    OP_F64_ADD,             /* add two f64 */
    OP_F64_SUB,             /* sub two f64 */
    OP_F64_MUL,             /* mul two f64 */
    OP_F64_DIV,             /* div two f64 */
    OP_F64_NEG,             /* negate f64  */

    OP_F64_EQ,              /* two f64 equal */
    OP_F64_NE,              /* two f64 not equal */
    OP_F64_GT,              /* two f64 greater than */
    OP_F64_GE,              /* two f64 greater equal */
    OP_F64_LT,              /* two f64 less than */
    OP_F64_LE,              /* two f64 less equal */

    OP_F64_RETURN,          /* return f64 value */

    /* (5) control flow */

    OP_JMP,                 // uncondition jump
    OP_JMP_INT32_EQ,
    OP_JMP_INT32_NE,
    OP_JMP_INT32_GT,
    OP_JMP_INT32_GE,
    OP_JMP_INT32_LT,
    OP_JMP_INT32_LE,
    OP_JMP_TRUE,
    OP_JMP_FALSE,

    OP_CALL,
    OP_CALL_ABSTRACT,
    OP_RETURN,

    /* (6) object/field operations */
    OP_NEW,

    OP_FIELD_I8_GET,
    OP_FIELD_I16_GET,
    OP_FIELD_I32_GET,
    OP_FIELD_I64_GET,
    OP_FIELD_F32_GET,
    OP_FIELD_F64_GET,
    OP_FIELD_SIZED_GET,
    OP_FIELD_REF_GET,

    OP_FIELD_I8_SET,
    OP_FIELD_I16_SET,
    OP_FIELD_I32_SET,
    OP_FIELD_I64_SET,
    OP_FIELD_F32_SET,
    OP_FIELD_F64_SET,
    OP_FIELD_SIZED_SET,
    OP_FIELD_REF_SET,

    OP_SIZED_RETURN,
    OP_REF_RETURN,

    /* (7) constants pool */
    OP_CONST_LOAD_I32,
    OP_CONST_LOAD_I64,

    /* (8) variable access */

    OP_LOCAL_I32_LOAD,
    OP_LOCAL_I32_STORE,
    OP_LOCAL_GET_I32,
    OP_LOCAL_SET_I32,

    OP_LOCAL_GET_I64,
    OP_LOCAL_SET_I64,

    OP_LOCAL_GET_F32,
    OP_LOCAL_SET_F32,

    OP_LOCAL_GET_F64,
    OP_LOCAL_SET_F64,

    OP_LOCAL_SIZED_GET,
    OP_LOCAL_SIZED_SET,
    OP_LOCAL_LABEL_GET,

    OP_LOCAL_REF_GET,
    OP_LOCAL_REF_SET,

    OP_GLOBAL_GET,
    OP_GLOBAL_SET,
    OP_GLOBAL_REF,

    /* (9) array operations */
    OP_ARRAY_I8_NEW,
    OP_ARRAY_I8_GET,
    OP_ARRAY_I8_SET,

    OP_ARRAY_I16_NEW,
    OP_ARRAY_I16_GET,
    OP_ARRAY_I16_SET,

    OP_ARRAY_I32_NEW,
    OP_ARRAY_I32_GET,
    OP_ARRAY_I32_SET,

    OP_ARRAY_I64_NEW,
    OP_ARRAY_I64_GET,
    OP_ARRAY_I64_SET,

    OP_ARRAY_F32_NEW,
    OP_ARRAY_F32_GET,
    OP_ARRAY_F32_SET,

    OP_ARRAY_F64_NEW,
    OP_ARRAY_F64_GET,
    OP_ARRAY_F64_SET,

    OP_ARRAY_SIZED_NEW,
    OP_ARRAY_SIZED_GET,
    OP_ARRAY_SIZED_SET,

    OP_ARRAY_REF_NEW,
    OP_ARRAY_REF_GET,
    OP_ARRAY_REF_SET,

    /* (10) switch table */

    /* (11) cast operations */
    OP_CAST_I32_TO_I8,
    OP_CAST_I32_TO_I16,
    // clang-format on
} KlOpCode;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPCODE_H_ */
