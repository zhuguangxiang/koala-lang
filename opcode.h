
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
	Load data from locvars to stack
	arg: 2 bytes, index of locvars
	-------------------------------------------------------
	val = load(arg)
	push(val)
 */
#define OP_LOAD   3

/*
	Store data from stack to locvars
	arg: 2 bytes, index of locvars
	-------------------------------------------------------
	val = pop()
	store(val, arg)
 */
#define OP_STORE  4

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
#define OP_GETFIELD 5

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
#define OP_SETFIELD 6

/*
	Call a function
	Parameters passed to function are already prepared in stack
	All parameters are pushed into stack from right to left
	arg0: 4 bytes, index of constant pool, function's name
	-------------------------------------------------------
	obj = top()
	func = get_func(obj, arg0)
	f = new_frame(func)
	push_func_stack(f)
	return
 */
#define OP_CALL 7

/*
	Function's return
	Return values are already stored in stack
	no args
 */
#define OP_RET  8

/*
	Arthmetic operation: add, sub, mul, div
	All args, include object, are in stack. Result is also saved in stack.
 */
#define OP_ADD  9
#define OP_SUB  10
#define OP_MUL  11
#define OP_DIV  12

/*
	New object
 */
#define OP_NEW    13
#define OP_STRING 14
#define OP_LIST   15
#define OP_TUPLE  16
#define OP_TABLE  17
#define OP_ARRAY  18

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

char *OPCode_ToString(uint8 op);
void Code_Show(uint8 *code, int32 size);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OPCODE_H_ */
