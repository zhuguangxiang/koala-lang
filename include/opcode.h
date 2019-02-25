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

/* Stop code frame loop */
#define OP_HALT 0x00

/* Pop an object */
#define OP_POP  0x09
/* Duplicate object and push it in stack */
#define OP_DUP  0x0a

/*
 * push byte integer to stack
 * size: 1 bytes
 */
#define OP_BPUSH 0x0b
/*
 * push short integer to stack
 * size: 2 bytes
 */
#define OP_SPUSH 0x0c

/*
 * Load constant from constants pool to stack
 * arg: index of constants pool
 * size: 2 bytes
 */
#define OP_LOADK 0x10

/*
 * Load package to stack
 * arg: index of constant pool, package's path
 * size: 2 bytes
 */
#define OP_LOADP 0x11

/*
 * Load data from local variables's vector to stack
 * arg: index of local variables' vector
 * size: 1 bytes
 */
#define OP_LOAD   0x12
#define OP_LOAD_0 0x13
#define OP_LOAD_1 0x14
#define OP_LOAD_2 0x15
#define OP_LOAD_3 0x16

/*
 * Store data from stack to local variables' vector
 * arg: index of local variables' vector
 * size: 1 bytes
 */
#define OP_STORE   0x17
#define OP_STORE_0 0x18
#define OP_STORE_1 0x19
#define OP_STORE_2 0x1a
#define OP_STORE_3 0x1b

/*
 * Get the field from object
 * arg: index of constant pool, field's name
 * size: 2 bytes
 * NOTE: object is stored in stack
 */
#define OP_LOAD_FIELD 0x1c

/*
 * Set the field of object
 * arg: index of constant pool, field's name
 * size: 2 bytes
 * NOTE: object is stored in stack
 */
#define OP_STORE_FIELD 0x1d

/*
 * Call a function
 * arg: number of arguments
 * size: 1 bytes
 * NOTE:
 *  Function is already stored in stack
 *  Parameters passed to function are already stored in stack
 *  All parameters are pushed into stack from right to left
 */
#define OP_CALL 0x1e

/*
 * Function's return
 * NOTE: Return values are already stored in stack
 */
#define OP_RETURN 0x1f

/* Binaray & Unary operations */
#define OP_ADD    0x20
#define OP_SUB    0x21
#define OP_MUL    0x22
#define OP_DIV    0x23
#define OP_MOD    0x24
#define OP_POWER  0x25
#define OP_NEG    0x26

#define OP_GT     0x27
#define OP_GE     0x28
#define OP_LT     0x29
#define OP_LE     0x2a
#define OP_EQ     0x2b
#define OP_NEQ    0x2c

#define OP_LAND   0x2d
#define OP_LOR    0x2e
#define OP_LNOT   0x2f

#define OP_BAND   0x30
#define OP_BOR    0x31
#define OP_BXOR   0x32
#define OP_BNOT   0x33
#define OP_LSHIFT 0x34
#define OP_RSHIFT 0x35

/* Control flow, with relative 2 bytes offset */
#define OP_JMP          0x40
#define OP_JMP_TRUE     0x41
#define OP_JMP_FALSE    0x42
#define OP_JMP_CMP_EQ   0x43
#define OP_JMP_CMP_NEQ  0x44
#define OP_JMP_CMP_LT   0x45
#define OP_JMP_CMP_GT   0x46
#define OP_JMP_CMP_LE   0x47
#define OP_JMP_CMP_GE   0x48
#define OP_JMP_NIL      0x49
#define OP_JMP_NOTNIL   0x4a

/* New object and access it */
#define OP_NEW          0x50
#define OP_NEW_ARRAY    0x51
#define OP_NEW_DICT     0x52
#define OP_NEW_SET      0x53
#define OP_NEW_CLOSURE  0x54
#define OP_ARRAY_LOAD   0x55
#define OP_ARRAY_STORE  0x56
#define OP_DICT_LOAD    0x57
#define OP_DICT_STORE   0x58

int OpCode_ArgCount(uint8 op);
char *OpCode_String(uint8 op);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OPCODE_H_ */
