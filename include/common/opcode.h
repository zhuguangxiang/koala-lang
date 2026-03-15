/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_OPCODE_H_
#define _KOALA_OPCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Standard 4-byte fixed-length instruction encoding formats:
 * Ax:   [op:8][a:12]
 * ABC:  [op:8][a:8][b:8][c:8]
 * AxBx: [op:8][ax:12][bx:12]
 * ABxx: [op:8][a:8][bxx:16]
 */
typedef enum { FORMAT_Ax, FORMAT_ABC, FORMAT_AxBx, FORMAT_ABxx } OpFormat;

typedef enum _OpCode {
#define X(name, fmt, cmt) name,
#include "opcode_list.h"
#undef X
    OP_MAX
} OpCode;

extern char *opcode_names[];
static inline char *opcode_name(OpCode code) { return opcode_names[code]; }
extern int opcode_formats[];
static inline int opcode_format(OpCode code) { return opcode_formats[code]; }

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPCODE_H_ */
