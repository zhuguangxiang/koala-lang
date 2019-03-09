/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _KOALA_OPCODE_H_
#define _KOALA_OPCODE_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Pop one object */
#define POP_TOP  1
/*
 * Pop #n objets
 * size: 1 bytes
 */
#define POP_TOPN 2

/* Duplicate top object and push it in stack */
#define DUP 3

/*
 * Push byte integer to stack
 * size: 1 bytes
 */
#define CONST_BYTE 4
/*
 * Push short integer to stack
 * size: 2 bytes
 */
#define CONST_SHORT 5

/*
 * Load constant from constants pool to stack
 * arg: index of constants pool
 * size: 2 bytes
 */
#define LOAD_CONST 10

/*
 * Load package to stack
 * arg: index of constant pool, package's path
 * size: 2 bytes
 */
#define LOAD_PKG 11

/*
 * Load data from local variables's vector to stack
 * arg: index of local variables' vector
 * size: 1 bytes
 */
#define LOAD   12

#define LOAD_0 13
#define LOAD_1 14
#define LOAD_2 15
#define LOAD_3 16

/*
 * Store data from stack to local variables' vector
 * arg: index of local variables' vector
 * size: 1 bytes
 */
#define STORE   17

#define STORE_0 18
#define STORE_1 19
#define STORE_2 20
#define STORE_3 21

/*
 * Get the field from object
 * arg: index of constant pool(field's name)
 * size: 2 bytes
 * NOTE: object is stored in stack
 */
#define LOAD_FIELD 22

/*
 * Set the field of object
 * arg: index of constant pool(field's name)
 * size: 2 bytes
 * NOTE: object is stored in stack
 */
#define STORE_FIELD 23

/*
 * Call a function/closure
 * arg0: number of arguments
 * arg1: index of constant pool(function's name)
 * size: 1+2 bytes
 * NOTE:
 *  Function is already stored in stack
 *  Parameters passed to function are already stored in stack
 *  All parameters are pushed into stack from right to left
 */
#define CALL 24

/*
 * Function's return
 * NOTE: Return values are already stored in stack
 */
#define RETURN 25

/* Binaray & Unary operations */
#define ADD 40
#define SUB 41
#define MUL 42
#define DIV 43
#define MOD 44
#define POW 45
#define NEG 46

#define GT  47
#define GE  48
#define LT  49
#define LE  50
#define EQ  51
#define NEQ 52

#define BAND   53
#define BOR    54
#define BXOR   55
#define BNOT   56
#define LSHIFT 57
#define RSHIFT 58

#define AND 59
#define OR  60
#define NOT 61

/* Control flow, with relative 2 bytes offset */
#define JMP        70
#define JMP_TRUE   71
#define JMP_FALSE  72
#define JMP_CMPEQ  73
#define JMP_CMPNEQ 74
#define JMP_CMPLT  75
#define JMP_CMPGT  76
#define JMP_CMPLE  77
#define JMP_CMPGE  78
#define JMP_NIL    79
#define JMP_NOTNIL 80

/* New object and access it */
#define NEW         90
#define NEW_ARRAY   91
#define NEW_MAP     92
#define NEW_SET     93
#define NEW_CLOSURE 94
#define MAP_LOAD    95
#define MAP_STORE   96

int OpCode_ArgCount(uint8 op);
char *OpCode_String(uint8 op);
char *OpCode_Operator(uint8 op);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OPCODE_H_ */
