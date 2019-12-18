/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef _KOALA_OPCODE_H_
#define _KOALA_OPCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define OP_HALT         0
#define OP_POP_TOP      1
#define OP_DUP          2
#define OP_SWAP         3
#define OP_CONST_BYTE   4
#define OP_CONST_SHORT  5
#define OP_CONST_NULL   6
#define OP_LOAD_CONST   7
#define OP_LOAD_MODULE  8

/* 9 - 9 */

#define OP_LOAD   10
#define OP_LOAD_0 11
#define OP_LOAD_1 12
#define OP_LOAD_2 13
#define OP_LOAD_3 14

#define OP_STORE   15
#define OP_STORE_0 16
#define OP_STORE_1 17
#define OP_STORE_2 18
#define OP_STORE_3 19

#define OP_GET_METHOD   20
#define OP_GET_VALUE    21
#define OP_SET_VALUE    22
#define OP_RETURN_VALUE 23
#define OP_RETURN       24
#define OP_CALL         25
#define OP_EVAL         26
#define OP_PRINT        27
#define OP_TYPEOF       28
#define OP_TYPECHECK    29

#define OP_ADD  30
#define OP_SUB  31
#define OP_MUL  32
#define OP_DIV  33
#define OP_MOD  34
#define OP_POW  35
#define OP_NEG  36

#define OP_GT   37
#define OP_GE   38
#define OP_LT   39
#define OP_LE   40
#define OP_EQ   41
#define OP_NEQ  42

#define OP_BIT_AND 43
#define OP_BIT_OR  44
#define OP_BIT_XOR 45
#define OP_BIT_NOT 46

#define OP_AND  47
#define OP_OR   48
#define OP_NOT  49

#define OP_INPLACE_ADD  50
#define OP_INPLACE_SUB  51
#define OP_INPLACE_MUL  52
#define OP_INPLACE_DIV  53
#define OP_INPLACE_MOD  54
#define OP_INPLACE_POW  55
#define OP_INPLACE_AND  56
#define OP_INPLACE_OR   57
#define OP_INPLACE_XOR  58

#define OP_SUBSCR_LOAD  59
#define OP_SUBSCR_STORE 60
#define OP_SLICE_LOAD   61
#define OP_SLICE_STORE  62

/* 63 - 69 */

#define OP_JMP        70
#define OP_JMP_TRUE   71
#define OP_JMP_FALSE  72
#define OP_JMP_CMPEQ  73
#define OP_JMP_CMPNEQ 74
#define OP_JMP_CMPLT  75
#define OP_JMP_CMPGT  76
#define OP_JMP_CMPLE  77
#define OP_JMP_CMPGE  78
#define OP_JMP_NIL    79
#define OP_JMP_NOTNIL 80

/* 81 - 84 */

#define OP_NEW_TUPLE    85
#define OP_NEW_ARRAY    86
#define OP_NEW_MAP      87
#define OP_NEW_RANGE    88
#define OP_NEW_CLOSURE  89
#define OP_NEW_EVAL     91
#define OP_NEW          92
#define OP_NEW_ITER     93

/* 94 - 95 */

#define OP_FOR_ITER     96
#define OP_UNPACK_TUPLE 97

/* 98 - 99 */

#define OP_UPVAL_LOAD   100
#define OP_UPVAL_STORE  101

#define OP_LOAD_GLOBAL  103
#define OP_INIT_CALL    104

#define OP_MATCH  105
#define OP_DOT_INDEX  106

#define OP_SUPER_CALL       107
#define OP_SUPER_INIT_CALL  108
#define OP_GET_SUPER_VALUE  109
#define OP_SET_SUPER_VALUE  110

int opcode_argc(int op);
char *opcode_str(int op);
char *opcode_map(int op);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPCODE_H_ */
