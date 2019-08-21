/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_OPCODE_H_
#define _KOALA_OPCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define POP_TOP 1
#define DUP     2

#define CONST_BYTE  3
#define CONST_SHORT 4

#define LOAD_CONST  5
#define LOAD_MODULE 6

#define LOAD   10
#define LOAD_0 11
#define LOAD_1 12
#define LOAD_2 13
#define LOAD_3 14

#define STORE   15
#define STORE_0 16
#define STORE_1 17
#define STORE_2 18
#define STORE_3 19

#define GET_FIELD  20
#define GET_METHOD 21
#define GET_FIELD_VALUE 22
#define SET_FIELD_VALUE 23
#define RETURN_VALUE 24
#define CALL 25
#define PRINT 26

#define ADD  30
#define SUB  31
#define MUL  32
#define DIV  33
#define MOD  34
#define POW  35
#define NEG  36

#define GT   37
#define GE   38
#define LT   39
#define LE   40
#define EQ   41
#define NEQ  42

#define BAND 43
#define BOR  44
#define BXOR 45
#define BNOT 46

#define AND  47
#define OR   48
#define NOT  49

#define JMP        50
#define JMP_TRUE   51
#define JMP_FALSE  52
#define JMP_CMPEQ  53
#define JMP_CMPNEQ 54
#define JMP_CMPLT  55
#define JMP_CMPGT  56
#define JMP_CMPLE  57
#define JMP_CMPGE  58
#define JMP_NIL    59
#define JMP_NOTNIL 60

#define NEW         70
#define NEW_ARRAY   71
#define NEW_TUPLE   72
#define NEW_SET     73
#define NEW_MAP     74
#define NEW_ITER    75
#define FOR_ITER    76
#define NEW_EVAL    77
#define NEW_CLOSURE 78
#define MAP_LOAD    79
#define MAP_STORE   80

int opcode_argc(int op);
char *opcode_str(int op);
char *opcode_operator(int op);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPCODE_H_ */
