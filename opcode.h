
#ifndef _KOALA_OPCODE_H_
#define _KOALA_OPCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
  Stop koala virtual machine
 */
#define OP_HALT   0

/*
  Load constant from constants pool to register
  args: index of constants pool with current method
 */
#define OP_LOADK  1

/*
  Load module with path to routine stack
  args: index of constant pool, module path
 */
#define OP_LOADM  2

/*
  Load data from local memory to register
  args: index of local variables with current method
 */
#define OP_LOAD   3

/*
  Store data from register to local memory
  args: index of local variables with current method
 */
#define OP_STORE  4

/*
  Get the field from module or object
  args: index of constant pool, field name
 */
#define OP_GETFIELD   5

/*
  Set the field of module or object
  args: index of constant pool, field name
 */
#define OP_SETFIELD   6

/*
  Call a function
  Parameters passed to function are already prepared in register
  args: method full path name index of constants pool
 */
#define OP_CALL   7

/*
  Function return
  Return values are stored in routine stack
 */
#define OP_RET    8

#define OP_GO     9

#define OP_NEW    11
#define OP_ARRAY  12

#define OP_ADD      21
#define OP_SUB      22
#define OP_MUL      23
#define OP_DIV      24

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OPCODE_H_ */
