/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_OPCODE_H_
#define _KOALA_OPCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

// OP_LOADK_SPECIAL's operand id for special constants
typedef enum _SpecialConst {
    K_NONE = 0,
    K_FALSE = 1,
    K_TRUE = 2,
    K_EMPTY_STR = 3,
    K_EMPTY_LIST = 4,
    K_EMPTY_DICT = 5,
    K_FLOAT_POS_ZERO = 6,
    K_FLOAT_NEG_ZERO = 7,
    K_FLOAT_NAN = 8,
    K_FLOAT_POS_INF = 9,
    K_FLOAT_NEG_INF = 10,
} SpecialConst;

/*
 * Standard 4-byte fixed-length instruction encoding formats:
 *
 * FORMAT_NONE:
 *   IR-only pseudo-instruction format.
 *   Instructions with this format are never encoded into VM bytecode.
 *   encode() must not be called on them.
 *   Used for high-level IR constructs (e.g. OP_IR_JMP4) that are lowered
 *   by codegen into one or more real VM instructions.
 *
 * FORMAT_Op:   [op:8][---:24]
 * FORMAT_Ax:   [op:8][---:12][ax:12]
 * FORMAT_Axx:  [op:8][---:8][axx:16]
 * FORMAT_Axxx: [op:8][axxx:24]
 * FORMAT_ABC:  [op:8][a:8][b:8][c:8]
 * FORMAT_AxBx: [op:8][ax:12][bx:12]
 * FORMAT_ABxx: [op:8][a:8][bxx:16]
 */
typedef enum {
    FORMAT_NONE,
    FORMAT_Op,
    FORMAT_Ax,
    FORMAT_Axx,
    FORMAT_Axxx,
    FORMAT_ABC,
    FORMAT_AxBx,
    FORMAT_ABxx
} OpFormat;

typedef enum _OpCode {
#define X(name, fmt, s0, s1) name,
#include "opcode_list.h"
#undef X
} OpCode;

extern char *opcode_names[];
static inline char *opcode_name(OpCode code) { return opcode_names[code]; }
extern OpFormat opcode_formats[];
static inline OpFormat opcode_format(OpCode code) { return opcode_formats[code]; }

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPCODE_H_ */
