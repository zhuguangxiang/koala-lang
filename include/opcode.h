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
 * Get the attribute(field and method) from object
 * arg: index of constant pool(attribute's name)
 * size: 2 bytes
 * NOTE: object is stored in stack
 */
#define GET_ATTR 22

/*
 * Set the attribute(field and method)  of object
 * arg: index of constant pool(attribute's name)
 * size: 2 bytes
 * NOTE: object is stored in stack
 */
#define SET_ATTR 23

/*
 * Call a function/closure
 * arg0: index of constant pool(function's name)
 * arg1: number of arguments
 * size: 2 + 1 bytes
 * NOTE:
 *  Parameters passed to function are already stored in stack
 *  from right to left.
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
#define NEG 45

#define GT  46
#define GE  47
#define LT  48
#define LE  49
#define EQ  50
#define NEQ 51

#define BAND   52
#define BOR    53
#define BXOR   54
#define BNOT   55

#define AND 56
#define OR  57
#define NOT 58

/* Control flow, with relative 2 bytes offset */
#define JMP        60
#define JMP_TRUE   61
#define JMP_FALSE  62
#define JMP_CMPEQ  63
#define JMP_CMPNEQ 64
#define JMP_CMPLT  65
#define JMP_CMPGT  66
#define JMP_CMPLE  67
#define JMP_CMPGE  68
#define JMP_NIL    69
#define JMP_NOTNIL 70

/* New object and access it */
#define NEW         80
#define NEW_ARRAY   81
#define NEW_MAP     82
#define NEW_SET     83
#define NEW_CLOSURE 84
#define MAP_LOAD    85
#define MAP_STORE   86

int OpCode_ArgCount(int op);
char *OpCode_String(int op);
char *OpCode_Operator(int op);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OPCODE_H_ */
