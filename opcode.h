
#ifndef _KOALA_OPCODE_H_
#define _KOALA_OPCODE_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
	Stop koala virtual machine
	no args
 */
#define OP_HALT 0

/*
	Load constant from constants pool to stack
	arg: 4 bytes, index of constants pool
	-------------------------------------------------------
	val = load(arg)
	push(val)
 */
#define OP_LOADK  1

/*
	Load module to stack
	arg: 4 bytes, index of constant pool, module's path
	-------------------------------------------------------
	val = load(arg)
	push(val)
 */
#define OP_LOADM  2

/*
	Get the module in which the klass is
	arg: it is loaded into stack
 */
#define OP_GETM   3

/*
	Load data from locvars to stack
	arg: 2 bytes, index of locvars
	-------------------------------------------------------
	val = load(arg)
	push(val)
 */
#define OP_LOAD   4

/*
	Store data from stack to locvars
	arg: 2 bytes, index of locvars
	-------------------------------------------------------
	val = pop()
	store(val, arg)
 */
#define OP_STORE  5

/*
	Get the field from object
	arg0: 4 bytes, index of constant pool, field name
	object is stored in stack
	-------------------------------------------------------
	obj = ... load_object()
	push(obj)
	obj = pop()
	val = getfield(obj, arg0)
	push(val)
 */
#define OP_GETFIELD 6

/*
	Set the field of object
	arg0: 4 bytes, index of constant pool, field name
	object is stored in stack
	-------------------------------------------------------
	val = ...
	push(val)
	obj = load_object()
	push(obj)
	obj = pop()
	val = pop()
	setfield(obj, arg0, val)
 */
#define OP_SETFIELD 7

/*
	Call a function
	Parameters passed to function are already prepared in stack
	All parameters are pushed into stack from right to left
	arg0: 4 bytes, index of constant pool, function's name
	arg1: 2 bytes, number of arguments
	-------------------------------------------------------
	obj = top()
	func = get_func(obj, arg0)
	f = new_frame(func)
	push_func_stack(f)
	return
 */
#define OP_CALL 8

/*
	Function's return
	Return values are already stored in stack
	no args
 */
#define OP_RET  9

/*
	Arthmetic operation: add, sub, mul, div
	All args, include object, are in stack. Result is also saved in stack.
 */
#define OP_ADD  10
#define OP_SUB  11
#define OP_MUL  12
#define OP_DIV  13

#define OP_PLUS  14
#define OP_MINUS 15
#define OP_BNOT  16
#define OP_LNOT  17

/*
	Relation operation: gt, lt, eq, gte, lte
 */
#define OP_GT  20
#define OP_GE  21
#define OP_LT  22
#define OP_LE  23
#define OP_EQ  24
#define OP_NEQ 25

/*
	Control flow, with relative 4 bytes offset
*/
#define OP_JUMP  30
#define OP_JUMP_TRUE  31
#define OP_JUMP_FALSE 32

/*
	New object: like OP_CALL
 */
#define OP_NEW  50

// #define OP_STRING 14
// #define OP_LIST   15
// #define OP_TUPLE  16
// #define OP_TABLE  17
// #define OP_ARRAY  18

/*
	Load constant from constant pool to locvars directly
	arg0: 4 bytes, index of constant pool
	arg1: 2 bytes, index of locvars
	-------------------------------------------------------
	val = load(arg0)
	store(val, arg1)
 */
#define OP2_LOADK 50

/*
	Get the field from object directly
	arg0: 4 bytes, index of constant pool, field name
	arg1: 2 bytes, index of object in locvars
	-------------------------------------------------------
	obj = load(arg1)
	val = getfield(obj, arg0)
	push(val)
 */
#define OP2_GETFIELD  51

/*
	Set the field to object directly
	arg0: 4 bytes, index of constant pool, field name
	arg1: 2 bytes, index of object in locvars
	-------------------------------------------------------
	val = ...
	push(val)
	val = pop()
	setfield(arg1, arg0, val)
 */
#define OP2_SETFIELD  52

/*
	Arthmetic operation: add, sub, mul, div
	Args are in locvars. Result is saved in stack.
 */
#define OP2_ADD   53
#define OP2_SUB   54
#define OP2_MUL   55
#define OP2_DIV   56

int opcode_argsize(uint8 op);
char *opcode_string(uint8 op);
void code_show(uint8 *code, int32 size);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OPCODE_H_ */
