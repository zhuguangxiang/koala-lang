/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KLVM_OPCODE_H_
#define _KLVM_OPCODE_H_

#if !defined(_KLVM_H_)
#error "Only <klvm/klvm.h> can be included directly."
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _KLVMOpCode {
    KLVM_OP_NONE,

    KLVM_OP_COPY,

    KLVM_OP_ADD,
    KLVM_OP_SUB,
    KLVM_OP_MUL,
    KLVM_OP_DIV,
    KLVM_OP_MOD,

    KLVM_OP_CMP_EQ,
    KLVM_OP_CMP_NE,
    KLVM_OP_CMP_GT,
    KLVM_OP_CMP_GE,
    KLVM_OP_CMP_LT,
    KLVM_OP_CMP_LE,

    KLVM_OP_AND,
    KLVM_OP_OR,

    KLVM_OP_BIT_AND,
    KLVM_OP_BIT_OR,
    KLVM_OP_BIT_XOR,
    KLVM_OP_BIT_SHL,
    KLVM_OP_BIT_SHR,

    KLVM_OP_NEG,
    KLVM_OP_NOT,
    KLVM_OP_BIT_NOT,

    KLVM_OP_CALL,
    KLVM_OP_CALL_VOID,

    KLVM_OP_RET,
    KLVM_OP_RET_VOID,
    KLVM_OP_JMP,
    KLVM_OP_COND_JMP,

    KLVM_OP_FIELD_GET,
    KLVM_OP_FIELD_SET,

    KLVM_OP_SUBSCR_GET,
    KLVM_OP_SUBSCR_SET,

    KLVM_OP_NEW_ARRAY,
    KLVM_OP_NEW_MAP,
    KLVM_OP_NEW_TUPLE,
    KLVM_OP_NEW,

    KLVM_OP_MAX,
} KLVMOpCode;

#ifdef __cplusplus
}
#endif

#endif /* _KLVM_OPCODE_H_ */
