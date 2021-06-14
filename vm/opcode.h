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

4 bytes instruction:

 0        7 0        7 0        7 0        7
+----------+----------+----------+----------+
|  opcode  |    A     |    B     |     C    |
+----------+----------+----------+----------+

5 bytes instruction:

 0        7 0        7 0        7 0        7 0        7
+----------+----------+----------+----------+----------+
|  0xFF    |  opcode  |    A     |    B     |     C    |
+----------+----------+----------+----------+----------+

The register index argument of opcode is 1 byte.
The constant pool argument is 4 bytes.
The Offset of jmp is 3 bytes.
Type is ref or fat-ref.

i8/i16 -> i32 auto

bool: op_i8_const 0/1
return i32

char: op_i32_const
return i32

~: -1 xor val

*/

/* clang-format off */

typedef enum {
/*----------------------------------------------------------------------------
    name                       arguments        description
----------------------------------------------------------------------------*/
    OP_MOVE,                /* A  B             R(A) = R(B)                 */
    OP_NIL,                 /* A                R(A) = nil                  */
    OP_I8K,                 /* A  K(1)          R(A) = (i8)K                */
    OP_I16K,                /* A  K(2)          R(A) = (i16)K               */
    OP_I32K,                /* A  K(4)          R(A) = (i32)K               */
    OP_F32K,                /* A  K(4)          R(A) = (f32)K               */
    OP_LDC_I64,             /* A  K(2)          R(A) = (i64)CP[K]           */
    OP_LDC_F64,             /* A  K(2)          R(A) = (f64)CP[K]           */
    OP_LDC_STR,             /* A  K(2)          R(A) = (Ref)CP[K]           */

    OP_I32_ADD,             /* A  B  C          R(A) = R(B) + R(C)          */
    OP_I32_SUB,             /* A  B  C          R(A) = R(B) - R(C)          */
    OP_I32_MUL,             /* A  B  C          R(A) = R(B) * R(C)          */
    OP_I32_DIV,             /* A  B  C          R(A) = R(B) / R(C)          */
    OP_I32_MOD,             /* A  B  C          R(A) = R(B) % R(C)          */
    OP_I32_NEG,             /* A  B             R(A) = -R(B)                */
    OP_I32_AND,             /* A  B  C          R(A) = R(B) & R(C)          */
    OP_I32_OR,              /* A  B  C          R(A) = R(B) | R(C)          */
    OP_I32_XOR,             /* A  B  C          R(A) = R(B) ^ R(C)          */
    OP_I32_SHL,             /* A  B  C          R(A) = R(B) << R(C)         */
    OP_I32_SHR,             /* A  B  C          R(A) = R(B) >> R(C)         */
    OP_I32_USHR,            /* A  B  C          R(A) = R(B) >>> R(C)        */
    OP_I32_CMP,             /* A  B  C          R(A) = 1/0/-1               */

    OP_I32_ADDK,            /* A  B  K(1)       R(A) = R(B) + (u8)K         */
    OP_I32_SUBK,            /* A  B  K(1)       R(A) = R(B) - (u8)K         */
    OP_I32_MULK,            /* A  B  K(1)       R(A) = R(B) * (u8)K         */
    OP_I32_DIVK,            /* A  B  K(1)       R(A) = R(B) / (u8)K         */
    OP_I32_MODK,            /* A  B  K(1)       R(A) = R(B) % (u8)K         */
    OP_I32_ANDK,            /* A  B  K(1)       R(A) = R(B) & (u8)K         */
    OP_I32_ORK,             /* A  B  K(1)       R(A) = R(B) | (u8)K         */
    OP_I32_XORK,            /* A  B  K(1)       R(A) = R(B) ^ (u8)K         */
    OP_I32_SHLK,            /* A  B  K(1)       R(A) = R(B) << (u8)K        */
    OP_I32_SHRK,            /* A  B  K(1)       R(A) = R(B) >> (u8)K        */
    OP_I32_USHRK,           /* A  B  K(1)       R(A) = R(B) >>> (u8)K       */
    OP_I32_CMPK,            /* A  B  K(1)       R(A) = 1/0/-1               */

    OP_I64_ADD,             /* A  B  C          R(A) = R(B) + R(C)          */
    OP_I64_SUB,             /* A  B  C          R(A) = R(B) - R(C)          */
    OP_I64_MUL,             /* A  B  C          R(A) = R(B) * R(C)          */
    OP_I64_DIV,             /* A  B  C          R(A) = R(B) / R(C)          */
    OP_I64_MOD,             /* A  B  C          R(A) = R(B) % R(C)          */
    OP_I64_NEG,             /* A  B             R(A) = -R(B)                */
    OP_I64_AND,             /* A  B  C          R(A) = R(B) & R(C)          */
    OP_I64_OR,              /* A  B  C          R(A) = R(B) | R(C)          */
    OP_I64_XOR,             /* A  B  C          R(A) = R(B) ^ R(C)          */
    OP_I64_SHL,             /* A  B  C          R(A) = R(B) << R(C)         */
    OP_I64_SHR,             /* A  B  C          R(A) = R(B) >> R(C)         */
    OP_I64_USHR,            /* A  B  C          R(A) = R(B) >>> R(C)        */
    OP_I64_CMP,             /* A  B  C          R(A) = 1/0/-1               */

    OP_I64_ADDK,            /* A  B  K(1)       R(A) = R(B) + (u8)K         */
    OP_I64_SUBK,            /* A  B  K(1)       R(A) = R(B) - (u8)K         */
    OP_I64_MULK,            /* A  B  K(1)       R(A) = R(B) * (u8)K         */
    OP_I64_DIVK,            /* A  B  K(1)       R(A) = R(B) / (u8)K         */
    OP_I64_MODK,            /* A  B  K(1)       R(A) = R(B) % (u8)K         */
    OP_I64_ANDK,            /* A  B  K(1)       R(A) = R(B) & (u8)K         */
    OP_I64_ORK,             /* A  B  K(1)       R(A) = R(B) | (u8)K         */
    OP_I64_XORK,            /* A  B  K(1)       R(A) = R(B) ^ (u8)K         */
    OP_I64_SHLK,            /* A  B  K(1)       R(A) = R(B) << (u8)K        */
    OP_I64_SHRK,            /* A  B  K(1)       R(A) = R(B) >> (u8)K        */
    OP_I64_USHRK,           /* A  B  K(1)       R(A) = R(B) >>> (u8)K       */
    OP_I64_CMPK,            /* A  B  K(1)       R(A) = 1/0/-1               */

    OP_F32_ADD,             /* A  B  C          R(A) = R(B) + R(C)          */
    OP_F32_SUB,             /* A  B  C          R(A) = R(B) - R(C)          */
    OP_F32_MUL,             /* A  B  C          R(A) = R(B) * R(C)          */
    OP_F32_DIV,             /* A  B  C          R(A) = R(B) / R(C)          */
    OP_F32_MOD,             /* A  B  C          R(A) = R(B) % R(C)          */
    OP_F32_NEG,             /* A  B             R(A) = -R(B)                */
    OP_F32_CMP,             /* A  B  C          R(A) = 1/0/-1               */

    OP_F64_ADD,             /* A  B  C          R(A) = R(B) + R(C)          */
    OP_F64_SUB,             /* A  B  C          R(A) = R(B) - R(C)          */
    OP_F64_MUL,             /* A  B  C          R(A) = R(B) * R(C)          */
    OP_F64_DIV,             /* A  B  C          R(A) = R(B) / R(C)          */
    OP_F64_MOD,             /* A  B  C          R(A) = R(B) % R(C)          */
    OP_F64_NEG,             /* A  B             R(A) = -R(B)                */
    OP_F64_CMP,             /* A  B  C          R(A) = 1/0/-1               */

    OP_JMP,                 /* K(2)             pc += K                     */
    OP_JEQ,                 /* A  K(2)          R(A) == 0, pc += K          */
    OP_JNE,                 /* A  K(2)          R(A) != 0, pc += K          */
    OP_JLT,                 /* A  K(2)          R(A) < 0,  pc += K          */
    OP_JLE,                 /* A  K(2)          R(A) <= 0, pc += K          */
    OP_JGT,                 /* A  K(2)          R(A) > 0,  pc += K          */
    OP_JGE,                 /* A  K(2)          R(A) >= 0, pc += K          */

    OP_RET,                 /*                  return                      */
    OP_RETV,                /* A                R(0) = R(A), return         */
    OP_RETK,                /* K(2)             R(0) = K, return            */

    OP_CALL,                /* K1(1) = argc  K2(2) = offset                 */
    OP_DYN_CALL,            /* K1(1) = argc  K2(2) = offset                 */

    OP_PUSH,                /* A                R(++top) = R(A)             */
    OP_PUSH_I32_ADD,        /* A  B             R(++top) = R(A) + R(B)      */
    OP_PUSH_I32_SUB,        /* A  B             R(++top) = R(A) - R(B)      */
    OP_PUSH_I32_ADDK,       /* A  K(1)          R(++top) = R(A) + (u8)K     */
    OP_PUSH_I32_SUBK,       /* A  K(1)          R(++top) = R(A) - (u8)K     */

    OP_SAVE_RET,            /* A                R(A) = R(top + 1)           */
    OP_I32_ADD_RET,         /* A  B             R(A) = R(top + 1) + R(B)    */
    OP_I32_SUB_RET,         /* A  B             R(A) = R(top + 1) - R(B)    */
    OP_I32_ADDK_RET,        /* A  K(1)          R(A) = R(top + 1) + (u8)K   */
    OP_I32_SUBK_RET,        /* A  K(1)          R(A) = R(top + 1) - (u8)K   */

    OP_I32_JMP_CMPEQ,       /* A  B  K(2)       R(A) == R(B), pc += K       */
    OP_I32_JMP_CMPNE,       /* A  B  K(2)       R(A) != R(B), pc += K       */
    OP_I32_JMP_CMPLT,       /* A  B  K(2)       R(A) < R(B), pc += K        */
    OP_I32_JMP_CMPLE,       /* A  B  K(2)       R(A) <= R(B), pc += K       */
    OP_I32_JMP_CMPGT,       /* A  B  K(2)       R(A) > R(B), pc += K        */
    OP_I32_JMP_CMPGE,       /* A  B  K(2)       R(A) >= R(B), pc += K       */

    OP_I32_JMP_CMPKEQ,      /* A  K1(1)  K2(2)  R(A) == K1, pc += K2        */
    OP_I32_JMP_CMPKNE,      /* A  K1(1)  K2(2)  R(A) != K1, pc += K2        */
    OP_I32_JMP_CMPKLT,      /* A  K1(1)  K2(2)  R(A) < K1, pc += K2         */
    OP_I32_JMP_CMPKLE,      /* A  K1(1)  K2(2)  R(A) <= K1, pc += K2        */
    OP_I32_JMP_CMPKGT,      /* A  K1(1)  K2(2)  R(A) > K1, pc += K2         */
    OP_I32_JMP_CMPKGE,      /* A  K1(1)  K2(2)  R(A) >= K1, pc += K2        */

} OpCode;

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPCODE_H_ */
