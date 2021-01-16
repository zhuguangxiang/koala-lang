/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_OPCODE_H_
#define _KOALA_OPCODE_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OP_HALT 0

#define OP_POP_TOP 1
#define OP_DUP     2
#define OP_SWAP    3

#define OP_CONST_INT8    4
#define OP_CONST_INT16   5
#define OP_CONST_INT32   6
#define OP_CONST_INT64   7
#define OP_CONST_FLOAT32 8
#define OP_CONST_FLOAT64 8
#define OP_CONST_TRUE    9
#define OP_CONST_FALSE   10
#define OP_CONST_CHAR    11
#define OP_CONST_STRING  12

#define OP_LOAD   20
#define OP_LOAD_0 21
#define OP_LOAD_1 22
#define OP_LOAD_2 23
#define OP_LOAD_3 24

#define OP_STORE   25
#define OP_STORE_0 26
#define OP_STORE_1 27
#define OP_STORE_2 28
#define OP_STORE_3 29

#define OP_ADD 40
#define OP_SUB 41
#define OP_MUL 42
#define OP_DIV 43
#define OP_MOD 44
#define OP_POW 45
#define OP_NEG 46

#define OP_GT 47
#define OP_GE 48
#define OP_LT 49
#define OP_LE 50
#define OP_EQ 51
#define OP_NE 52

#define OP_AND 53
#define OP_OR  54
#define OP_NOT 55

#define OP_BIT_AND    56
#define OP_BIT_OR     57
#define OP_BIT_XOR    58
#define OP_BIT_NOT    59
#define OP_BIT_LSHIFT 60
#define OP_BIT_RSHIFT 61

#define OP_INPLACE_ADD    62
#define OP_INPLACE_SUB    63
#define OP_INPLACE_MUL    64
#define OP_INPLACE_DIV    65
#define OP_INPLACE_MOD    66
#define OP_INPLACE_POW    67
#define OP_INPLACE_AND    68
#define OP_INPLACE_OR     69
#define OP_INPLACE_XOR    70
#define OP_INPLACE_LSHIFT 71
#define OP_INPLACE_RSHIFT 72

#define OP_PRINT        80
#define OP_CALL         81
#define OP_RETURN       82
#define OP_RETURN_VALUE 83

char *opcode_str(uint8_t op);
uint8_t opcode_argc(uint8_t op);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPCODE_H_ */
