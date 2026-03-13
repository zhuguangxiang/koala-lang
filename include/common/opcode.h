/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_OPCODE_H_
#define _KOALA_OPCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */

typedef enum _OpCode {
/*+-----------------------------+-----------------------------------------------------+*/
/*| name                        |  format comments                                    |*/
/*+-----------------------------+-----------------------------------------------------+*/
    OP_NOP,                     /* [op:8][0:24]                                        */
    OP_MOVE,                    /* [op:8][Rd:12][Rs:12]         Rd = Rs                */
    OP_CAST_INTF,               /* [op:8][Rd:8][Rs:8][Idx:8]    Rd = (intf)Rs          */

    OP_CONST,                   /* [op:8][Rd:12][Idx:12]        Rd = CP[Idx]           */
    OP_CONST_NONE,              /* [op:8][Rd:12][0:12]          Rd = None              */
    OP_CONST_FALSE,             /* [op:8][Rd:12][0:12]          Rd = False             */
    OP_CONST_TRUE,              /* [op:8][Rd:12][0:12]          Rd = True              */
    OP_CONST_EMPTY_STR,         /* [op:8][Rd:12][0:12]          Rd = ""                */
    OP_CONST_EMPTY_LIST,        /* [op:8][Rd:12][0:12]          Rd = []                */
    OP_CONST_EMPTY_DICT,        /* [op:8][Rd:12][0:12]          Rd = {}                */

    OP_CONST_INT_M1,            /* [op:8][Rd:12][0:12]          Rd = -1                */
    OP_CONST_INT_0,             /* [op:8][Rd:12][0:12]          Rd = 0                 */
    OP_CONST_INT_1,             /* [op:8][Rd:12][0:12]          Rd = 1                 */
    OP_CONST_INT_IMM,           /* [op:8][Rd:8][Imm:16]         Rd = Imm               */

    OP_CONST_FLOAT_0,           /* [op:8][Rd:12][0:12]          Rd = 0.0               */
    OP_CONST_FLOAT_N0,          /* [op:8][Rd:12][0:12]          Rd = -0.0              */
    OP_CONST_FLOAT_NAN,         /* [op:8][Rd:12][0:12]          Rd = float('nan')      */
    OP_CONST_FLOAT_INF,         /* [op:8][Rd:12][0:12]          Rd = float('inf')      */
    OP_CONST_FLOAT_NINF,        /* [op:8][Rd:12][0:12]          Rd = float('-inf')     */

    OP_INT_ADD,             /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs + Rt           */
    OP_INT_SUB,             /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs - Rt           */
    OP_INT_MUL,             /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs * Rt           */
    OP_INT_DIV,             /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs / Rt           */
    OP_INT_MOD,             /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs % Rt           */
    OP_INT_NEG,             /* [op:8][Rd:8][Rs:8][0:8]   Rd = -Rs               */

    OP_INT_AND,             /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs & Rt           */
    OP_INT_OR,              /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs | Rt           */
    OP_INT_XOR,             /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs ^ Rt           */
    OP_INT_NOT,             /* [op:8][Rd:8][Rs:8][0:8]   Rd = ~Rs               */
    OP_INT_SHL,             /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs << Rt          */
    OP_INT_SHR,             /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs >> Rt          */

    OP_INT_CMP_EQ,          /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs == Rt          */
    OP_INT_CMP_NE,          /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs != Rt          */
    OP_INT_CMP_LT,          /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs < Rt           */
    OP_INT_CMP_GT,          /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs > Rt           */
    OP_INT_CMP_LE,          /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs <= Rt          */
    OP_INT_CMP_GE,          /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs >= Rt          */

    OP_INT_ADD_IMM,         /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs + Imm         */
    OP_INT_SUB_IMM,         /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs - Imm         */
    OP_INT_MUL_IMM,         /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs * Imm         */
    OP_INT_DIV_IMM,         /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs / Imm         */
    OP_INT_MOD_IMM,         /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs % Imm         */

    OP_INT_AND_IMM,         /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs & Imm         */
    OP_INT_OR_IMM,          /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs | Imm         */
    OP_INT_XOR_IMM,         /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs ^ Imm         */
    OP_INT_SHL_IMM,         /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs << Imm        */
    OP_INT_SHR_IMM,         /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs >> Imm        */

    OP_INT_CMP_EQ_IMM,      /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs == Imm        */
    OP_INT_CMP_NE_IMM,      /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs != Imm        */
    OP_INT_CMP_LT_IMM,      /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs < Imm         */
    OP_INT_CMP_GT_IMM,      /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs > Imm         */
    OP_INT_CMP_LE_IMM,      /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs <= Imm        */
    OP_INT_CMP_GE_IMM,      /* [op:8][Rd:8][Rs:8][Imm:8]  Rd = Rs >= Imm        */

    OP_FLOAT_ADD,           /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs + Rt           */
    OP_FLOAT_SUB,           /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs - Rt           */
    OP_FLOAT_MUL,           /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs * Rt           */
    OP_FLOAT_DIV,           /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs / Rt           */
    OP_FLOAT_MOD,           /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs % Rt           */
    OP_FLOAT_NEG,           /* [op:8][Rd:8][Rs:8][0:8]   Rd = -Rs               */

    OP_FLOAT_CMPL,          /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = (Rs < Rt) ? -1 : ((Rs == Rt) ? 0 : 1) */
    OP_FLOAT_CMPG,          /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = (Rs > Rt) ? 1 : ((Rs == Rt) ? 0 : -1) */

    OP_UINT_CMP_LT,         /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = (Rs < Rt) ? 1 : 0  */
    OP_UINT_CMP_LE,         /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = (Rs <= Rt) ? 1 : 0 */
    OP_UINT_CMP_GT,         /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = (Rs > Rt) ? 1 : 0  */
    OP_UINT_CMP_GE,         /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = (Rs >= Rt) ? 1 : 0 */

    OP_FLOAT_TO_INT,        /* [op:8][Rd:12][Rs:12]      Rd = (int)Rs            */
    OP_INT_TO_FLOAT,        /* [op:8][Rd:12][Rs:12]      Rd = (float)Rs          */

    /* logic operations */
    OP_LAND,                /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs && Rt          */
    OP_LOR,                 /* [op:8][Rd:8][Rs:8][Rt:8]  Rd = Rs || Rt          */
    OP_LNOT,                /* [op:8][Rd:12][Rs:12]      Rd = !Rs               */

    /* jump operations */
    OP_JMP,                     /* [op:8][0:8][Offset:16]    PC += Offset                     */
    OP_JMP_TRUE,                /* [op:8][A:8][Offset:16]    PC += Offset if R(A) is true     */
    OP_JMP_FALSE,               /* [op:8][A:8][Offset:16]    PC += Offset if R(A) is false    */
    OP_JMP_NONE,                /* [op:8][A:8][Offset:16]    PC += Offset if R(A) is None     */
    OP_JMP_NOT_NONE,            /* [op:8][A:8][Offset:16]    PC += Offset if R(A) is not None */

    OP_JMP_CMP_EQ,              /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) == R(B) */
    OP_JMP_CMP_NE,              /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) != R(B) */
    OP_JMP_CMP_LT,              /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) < R(B)  */
    OP_JMP_CMP_GT,              /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) > R(B)  */
    OP_JMP_CMP_LE,              /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) <= R(B) */
    OP_JMP_CMP_GE,              /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) >= R(B) */

    OP_JMP_INT_CMP_EQ,          /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) == R(B) */
    OP_JMP_INT_CMP_NE,          /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) != R(B) */
    OP_JMP_INT_CMP_LT,          /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) < R(B)  */
    OP_JMP_INT_CMP_GT,          /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) > R(B)  */
    OP_JMP_INT_CMP_LE,          /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) <= R(B) */
    OP_JMP_INT_CMP_GE,          /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) >= R(B) */

    OP_JMP_INT_CMP_EQ_IMM,      /* [op:8][A:8][imm:8][offset:8]    PC += Offset if R(A) == Imm */
    OP_JMP_INT_CMP_NE_IMM,      /* [op:8][A:8][imm:8][offset:8]    PC += Offset if R(A) != Imm */
    OP_JMP_INT_CMP_LT_IMM,      /* [op:8][A:8][imm:8][offset:8]    PC += Offset if R(A) < Imm  */
    OP_JMP_INT_CMP_GT_IMM,      /* [op:8][A:8][imm:8][offset:8]    PC += Offset if R(A) > Imm  */
    OP_JMP_INT_CMP_LE_IMM,      /* [op:8][A:8][imm:8][offset:8]    PC += Offset if R(A) <= Imm */
    OP_JMP_INT_CMP_GE_IMM,      /* [op:8][A:8][imm:8][offset:8]    PC += Offset if R(A) >= Imm */

    OP_JMP_UINT_CMP_EQ,         /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) == R(B) */
    OP_JMP_UINT_CMP_NE,         /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) != R(B) */
    OP_JMP_UINT_CMP_LT,         /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) < R(B)  */
    OP_JMP_UINT_CMP_LE,         /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) <= R(B) */
    OP_JMP_UINT_CMP_GT,         /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) > R(B)  */
    OP_JMP_UINT_CMP_GE,         /* [op:8][A:8][B:8][Offset:8]    PC += Offset if R(A) >= R(B) */

    /* call & argument pass */
    OP_PUSH,                    /* [op:8][0:12][Rs:12]      */
    OP_PUSH_INT_IMM,            /* [op:8][0:8][Imm:16]      */
    OP_PUSH_CONST,              /* [op:8][0:12][Idx:12]     */
    OP_PUSH_RELOC,              /* [op:8][0:8][Offset:16]   */
    OP_PUSH_2,                  /* [op:8][Rs1:12][Rs2:12]   */
    OP_PUSH_NONE,               /* [op:8][0:8][0:8][0:8]    */
    OP_PUSH_TRUE,               /* [op:8][0:8][0:8][0:8]    */
    OP_PUSH_FALSE,              /* [op:8][0:8][0:8][0:8]    */
    OP_PUSH_EMPTY_STR,          /* [op:8][0:8][0:8][0:8]    */
    OP_PUSH_EMPTY_LIST,         /* [op:8][0:8][0:8][0:8]    */
    OP_PUSH_EMPTY_DICT,         /* [op:8][0:8][0:8][0:8]    */

    OP_CALL,                    /* [op:8][Rd:8][imm:8][offset:8] */
    OP_CALL_KW,                 /* [op:8][Rd:8][imm:8][offset:8] */
    OP_CALL_DYNAMIC,            /* [op:8][Rd:8][imm:8][offset:8] */
    OP_CALL_DYNAMIC_KW,         /* [op:8][Rd:8][imm:8][offset:8] */

    /* return */
    OP_RETURN,                  /* [op:8][0:12][Rs:12]      return R(Rs) */
    OP_RETURN_NONE,             /* [op:8][0:8][0:8][0:8]    return None  */

    /* cast operation */
    OP_AS,
    OP_IS,

    /* globals */
    OP_GET_GLOBAL,              /* [op:8][Rd:12][offset:12] */
    OP_SET_GLOBAL,              /* [op:8][Rd:12][offset:12] */

    /* fields */
    OP_GET_FIELD,               /* [op:8][Rd:12][Rs:12][offset:8]   Rd = R(Rs).field[offset]    */
    OP_SET_FIELD,               /* [op:8][Rd:12][Rs:12][offset:8]   R(Rs).field[offset] = R(Rd) */

    /* generic subscript operations */
    OP_SUBSCR_LOAD,
    OP_SUBSCR_STORE,

    /* generic iterator operations */
    OP_GET_ITER,
    OP_ITER_NEXT,

    /* generic operators */

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

    OP_BINARY_CMP_EQ,
    OP_BINARY_CMP_NE,
    OP_BINARY_CMP_LT,
    OP_BINARY_CMP_GT,
    OP_BINARY_CMP_LE,
    OP_BINARY_CMP_GE,

    /* raise an error */
    OP_RAISE,

    /* extend op */
    OP_WIDE,

    /* The below insns are only in IR */
    OP_IR_LOCAL,
    OP_IR_PHI,
    OP_IR_JMP_COND,
} OpCode;

/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPCODE_H_ */
