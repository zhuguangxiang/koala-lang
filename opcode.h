
#ifndef _KOALA_OPCODE_H_
#define _KOALA_OPCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define OP_LOADK    1
#define OP_LOAD     2
#define OP_STORE    3

#define OP_CALL     10
#define OP_RET      11
#define OP_GO       12
#define OP_NEW        13
#define OP_NEWARRAY   14

#define OP_ADD      21
#define OP_SUB      22

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OPCODE_H_ */
