
#ifndef _KOALA_OPCODE_H_
#define _KOALA_OPCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
  Load constant from constants pool to register
  args: index of constants pool with current method
 */
#define OP_LOADK  1

/*
  Load data from local memory to register
  args: index of local variables with current method
 */
#define OP_LOAD   2

/*
  Store data from register to local memory
  args: index of local variables with current method
 */
#define OP_STORE  3

/*
  Get the module object with its path to register
  args: module path index of constants pool
 */
#define OP_GET_MODULE 4

/*
  Call a function
  Parameters passed to function are already prepared in register
  args:
    method full path name index of constants pool
 */
#define OP_CALL   5
#define OP_RET    6
#define OP_GO     7

#define OP_NEW
#define OP_NEWARRAY   14

#define OP_ADD      21
#define OP_SUB      22

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OPCODE_H_ */
