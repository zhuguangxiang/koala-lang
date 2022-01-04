/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_OPCODE_H_
#define _KOALA_OPCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*

instruction format:

opcode A, B, C
------------------------------------
A: target register index
If A[7] == 0, A is 8 bytes(0-127); if A[7] == 1, A is 16 bytes(128-32767).
B/C src register index or constant.

*/

typedef enum _KlOpCode KlOpCode;

// clang-format off

enum _KlOpCode {
//  name                    arguments           description
    OP_HALT,                //                  Halt VM

    OP_PUSH,                // A                R(++top) = R(A)
    OP_PUSH_I8,             // K(1)             R(++top) = K
    OP_PUSH_I16,            // K(2)             R(++top) = K
    OP_PUSH_I32,            // K(4)             R(++top) = K
    OP_PUSH_I64,            // K(2)             R(++top) = CP[K]
    OP_PUSH_F32,            // K(4)             R(++top) = K
    OP_PUSH_F64,            // K(2)             R(++top) = CP[K]
    OP_PUSH_BOOL,           // K(1)             R(++top) = K
    OP_PUSH_CHAR,           // K(4)             R(++top) = K
    OP_PUSH_STR,            // K(2)             R(++top) = CP[K]

    OP_MOV_TOP,             // A                R(A) = R(top--)
    OP_MOV,                 // A  B             R(A) = R(B)
    OP_CONST_I8,            // A  K(1)          R(A) = (i8)K
    OP_CONST_I16,           // A  K(2)          R(A) = (i16)K
    OP_CONST_I32,           // A  K(4)          R(A) = (i32)K
    OP_CONST_I64,           // A  K(2)          R(A) = CP[K]
    OP_CONST_F32,           // A  K(4)          R(A) = (f32)K
    OP_CONST_F64,           // A  K(2)          R(A) = CP[K]
    OP_CONST_BOOL,          // A  K(1)          R(A) = (i8)K
    OP_CONST_CHAR,          // A  K(1)          R(A) = (i32)K
    OP_CONST_STR,           // A  K(2)          R(A) = CP[K]

    OP_I32_ADD,             // A  B  C          R(A) = R(B) + R(C)
    OP_I32_SUB,             // A  B  C          R(A) = R(B) - R(C)
    OP_I32_MUL,             // A  B  C          R(A) = R(B) * R(C)
    OP_I32_DIV,             // A  B  C          R(A) = R(B) / R(C)
    OP_I32_MOD,             // A  B  C          R(A) = R(B) % R(C)
    OP_I32_NEG,             // A  B             R(A) = -R(B)

    OP_I32_AND,             // A  B  C          R(A) = R(B) & R(C)
    OP_I32_OR,              // A  B  C          R(A) = R(B) | R(C)
    OP_I32_XOR,             // A  B  C          R(A) = R(B) ^ R(C)
    OP_I32_NOT,             // A  B             R(A) = ~R(B)
    OP_I32_SHL,             // A  B  C          R(A) = R(B) << R(C)
    OP_I32_SHR,             // A  B  C          R(A) = R(B) >> R(C)
    OP_I32_USHR,            // A  B  C          R(A) = R(B) >>> R(C)

    OP_I32_ADD_I8,          // A  B  K(1)       R(A) = R(B) + (i8)K
    OP_I32_SUB_I8,          // A  B  K(1)       R(A) = R(B) - (i8)K
    OP_I32_MUL_I8,          // A  B  K(1)       R(A) = R(B) * (i8)K
    OP_I32_DIV_I8,          // A  B  K(1)       R(A) = R(B) / (i8)K
    OP_I32_MOD_I8,          // A  B  K(1)       R(A) = R(B) % (i8)K

    OP_I32_AND_I8,          // A  B  K(1)       R(A) = R(B) & (i8)K
    OP_I32_OR_I8,           // A  B  K(1)       R(A) = R(B) | (i8)K
    OP_I32_XOR_I8,          // A  B  K(1)       R(A) = R(B) ^ (i8)K
    OP_I32_SHL_I8,          // A  B  K(1)       R(A) = R(B) << (i8)K
    OP_I32_SHR_I8,          // A  B  K(1)       R(A) = R(B) >> (i8)K
    OP_I32_USHR_I8,         // A  B  K(1)       R(A) = R(B) >>> (i8)K

    OP_I32_EQ,              // A  B  C          R(A) = R(B) == R(C)
    OP_I32_NE,              // A  B  C          R(A) = R(B) != R(C)
    OP_I32_GT,              // A  B  C          R(A) = R(B) > R(C)
    OP_I32_GE,              // A  B  C          R(A) = R(B) >= R(C)
    OP_I32_LT,              // A  B  C          R(A) = R(B) < R(C)
    OP_I32_LE,              // A  B  C          R(A) = R(B) <= R(C)

    OP_I32_EQ_I8,           // A  B  K(1)       R(A) = R(B) == (i8)K
    OP_I32_NE_I8,           // A  B  K(1)       R(A) = R(B) != (i8)K
    OP_I32_GT_I8,           // A  B  K(1)       R(A) = R(B) > (i8)K
    OP_I32_GE_I8,           // A  B  K(1)       R(A) = R(B) >= (i8)K
    OP_I32_LT_I8,           // A  B  K(1)       R(A) = R(B) < (i8)K
    OP_I32_LE_I8,           // A  B  K(1)       R(A) = R(B) <= (i8)K

    OP_I64_ADD,             // A  B  C          R(A) = R(B) + R(C)
    OP_I64_SUB,             // A  B  C          R(A) = R(B) - R(C)
    OP_I64_MUL,             // A  B  C          R(A) = R(B) * R(C)
    OP_I64_DIV,             // A  B  C          R(A) = R(B) / R(C)
    OP_I64_MOD,             // A  B  C          R(A) = R(B) % R(C)
    OP_I64_NEG,             // A  B             R(A) = -R(B)

    OP_I64_AND,             // A  B  C          R(A) = R(B) & R(C)
    OP_I64_OR,              // A  B  C          R(A) = R(B) | R(C)
    OP_I64_XOR,             // A  B  C          R(A) = R(B) ^ R(C)
    OP_I64_NOT,             // A  B             R(A) = ~R(B)
    OP_I64_SHL,             // A  B  C          R(A) = R(B) << R(C)
    OP_I64_SHR,             // A  B  C          R(A) = R(B) >> R(C)
    OP_I64_USHR,            // A  B  C          R(A) = R(B) >>> R(C)

    OP_I64_ADD_I8,          // A  B  K(1)       R(A) = R(B) + (i8)K
    OP_I64_SUB_I8,          // A  B  K(1)       R(A) = R(B) - (i8)K
    OP_I64_MUL_I8,          // A  B  K(1)       R(A) = R(B) * (i8)K
    OP_I64_DIV_I8,          // A  B  K(1)       R(A) = R(B) / (i8)K
    OP_I64_MOD_I8,          // A  B  K(1)       R(A) = R(B) % (i8)K

    OP_I64_AND_I8,          // A  B  K(1)       R(A) = R(B) & (i8)K
    OP_I64_OR_I8,           // A  B  K(1)       R(A) = R(B) | (i8)K
    OP_I64_XOR_I8,          // A  B  K(1)       R(A) = R(B) ^ (i8)K
    OP_I64_SHL_I8,          // A  B  K(1)       R(A) = R(B) << (i8)K
    OP_I64_SHR_I8,          // A  B  K(1)       R(A) = R(B) >> (i8)K
    OP_I64_USHR_I8,         // A  B  K(1)       R(A) = R(B) >>> (i8)K

    OP_I64_EQ,              // A  B  C          R(A) = R(B) == R(C)
    OP_I64_NE,              // A  B  C          R(A) = R(B) != R(C)
    OP_I64_GT,              // A  B  C          R(A) = R(B) > R(C)
    OP_I64_GE,              // A  B  C          R(A) = R(B) >= R(C)
    OP_I64_LT,              // A  B  C          R(A) = R(B) < R(C)
    OP_I64_LE,              // A  B  C          R(A) = R(B) <= R(C)

    OP_I64_EQ_I8,           // A  B  K(1)       R(A) = R(B) == (i8)K
    OP_I64_NE_I8,           // A  B  K(1)       R(A) = R(B) != (i8)K
    OP_I64_GT_I8,           // A  B  K(1)       R(A) = R(B) > (i8)K
    OP_I64_GE_I8,           // A  B  K(1)       R(A) = R(B) >= (i8)K
    OP_I64_LT_I8,           // A  B  K(1)       R(A) = R(B) < (i8)K
    OP_I64_LE_I8,           // A  B  K(1)       R(A) = R(B) <= (i8)K

    OP_F32_ADD,             // A  B  C          R(A) = R(B) + R(C)
    OP_F32_SUB,             // A  B  C          R(A) = R(B) - R(C)
    OP_F32_MUL,             // A  B  C          R(A) = R(B) * R(C)
    OP_F32_DIV,             // A  B  C          R(A) = R(B) / R(C)
    OP_F32_MOD,             // A  B  C          R(A) = R(B) % R(C)
    OP_F32_NEG,             // A  B             R(A) = -R(B)

    OP_F32_EQ,              // A  B  C          R(A) = R(B) == R(C)
    OP_F32_NE,              // A  B  C          R(A) = R(B) != R(C)
    OP_F32_GT,              // A  B  C          R(A) = R(B) > R(C)
    OP_F32_GE,              // A  B  C          R(A) = R(B) >= R(C)
    OP_F32_LT,              // A  B  C          R(A) = R(B) < R(C)
    OP_F32_LE,              // A  B  C          R(A) = R(B) <= R(C)

    OP_F32_EQ_I8,           // A  B  K(1)       R(A) = R(B) == (i8)K
    OP_F32_NE_I8,           // A  B  K(1)       R(A) = R(B) != (i8)K
    OP_F32_GT_I8,           // A  B  K(1)       R(A) = R(B) > (i8)K
    OP_F32_GE_I8,           // A  B  K(1)       R(A) = R(B) >= (i8)K
    OP_F32_LT_I8,           // A  B  K(1)       R(A) = R(B) < (i8)K
    OP_F32_LE_I8,           // A  B  K(1)       R(A) = R(B) <= (i8)K

    OP_F64_ADD,             // A  B  C          R(A) = R(B) + R(C)
    OP_F64_SUB,             // A  B  C          R(A) = R(B) - R(C)
    OP_F64_MUL,             // A  B  C          R(A) = R(B) * R(C)
    OP_F64_DIV,             // A  B  C          R(A) = R(B) / R(C)
    OP_F64_MOD,             // A  B  C          R(A) = R(B) % R(C)
    OP_F64_NEG,             // A  B             R(A) = -R(B)

    OP_F64_EQ,              // A  B  C          R(A) = R(B) == R(C)
    OP_F64_NE,              // A  B  C          R(A) = R(B) != R(C)
    OP_F64_GT,              // A  B  C          R(A) = R(B) > R(C)
    OP_F64_GE,              // A  B  C          R(A) = R(B) >= R(C)
    OP_F64_LT,              // A  B  C          R(A) = R(B) < R(C)
    OP_F64_LE,              // A  B  C          R(A) = R(B) <= R(C)

    OP_F64_EQ_I8,           // A  B  K(1)       R(A) = R(B) == (i8)K
    OP_F64_NE_I8,           // A  B  K(1)       R(A) = R(B) != (i8)K
    OP_F64_GT_I8,           // A  B  K(1)       R(A) = R(B) > (i8)K
    OP_F64_GE_I8,           // A  B  K(1)       R(A) = R(B) >= (i8)K
    OP_F64_LT_I8,           // A  B  K(1)       R(A) = R(B) < (i8)K
    OP_F64_LE_I8,           // A  B  K(1)       R(A) = R(B) <= (i8)K

    OP_JMP,                 // K(2)             pc += K
    OP_JMP_TRUE,            // A, K(2)          pc += K
    OP_JMP_FALSE,           // A, K(2)          pc += K

    OP_LAND,                // A  B  C          R(A) = R(B) && R(C)
    OP_LOR,                 // A  B  C          R(A) = R(B) || R(C)
    OP_LNOT,                // A  B             R(A) = !R(B)

    OP_CALL,                // K1(1) K2(2)      argc = K1, offset = K2
    OP_RET_VOID,            //                  Return Void
    OP_RET,                 // A                R(0) = R(A), RET
    OP_RET_I8,              // K(1)             R(0) = K
    OP_RET_I16,             // K(2)             R(0) = K
    OP_RET_I32,             // K(4)             R(0) = K
    OP_RET_I64,             // K(2)             R(0) = CP[K]
    OP_RET_F32,             // K(4)             R(0) = K
    OP_RET_F64,             // K(2)             R(0) = CP[K]
    OP_RET_BOOL,            // K(1)             R(0) = K
    OP_RET_CHAR,            // K(4)             R(0) = K
    OP_RET_STR,             // K(2)             R(0) = CP[K]

    OP_NEW,
    OP_NEW_OPTION,
    OP_NEW_RESULT,
    OP_NEW_STRING,
    OP_NEW_RANGE,
    OP_NEW_ARRAY,
    OP_NEW_MAP,
    OP_NEW_TUPLE,

    OP_FIELD_GET,
    OP_FIELD_SET,
    OP_INDEX_GET,
    OP_INDEX_SET,
    OP_MAP_GET,
    OP_MAP_SET,
};

// clang-format on

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPCODE_H_ */
