/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_OPCODE_H_
#define _KOALA_OPCODE_H_

#include <stddef.h>
#include <stdint.h>
#include "opcode_only.h"

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

// clang-format off

/*
 * Standard 4-byte fixed-length instruction encoding formats:
 *
 * FORMAT_IR:
 *   IR-only pseudo-instruction format.
 *   Instructions with this format are never encoded into VM bytecode.
 *   encode() must not be called on them.
 *   Used for high-level IR constructs (e.g. OP_IR_JMP4) that are lowered
 *   by codegen into one or more real VM instructions.
 *
 * FORMAT_DATA:
 *   Linearized non-executable data slot format.
 *   Used for OP_DATA entries inserted after certain Mach ops (e.g. CALL)
 *   to carry auxiliary payloads such as type info, metadata, or extended
 *   operands. Not encoded as a normal instruction; emitted as raw data.
 *   The VM never executes FORMAT_DATA entries.
 *
 * FORMAT_Op:   [op:8][---:24]
 * FORMAT_Ax:   [op:8][ax:12][---:12]
 * FORMAT_Axx:  [op:8][axx:16][---:8]
 * FORMAT_ABC:  [op:8][a:8][b:8][c:8]
 * FORMAT_AxBx: [op:8][ax:12][bx:12]
 * FORMAT_ABxx: [op:8][a:8][bxx:16]
 *
 * FORMAT_CALL:
 *   Unified call instruction format (8 bytes total).
 *   First word:
 *       [op:8][flag:4][ret-reg:12][nargs:8]
 *   Second word:
 *       payload (32-bit), interpreted by flag:
 *           flag = 0 → relative-offset (signed 32-bit)
 *           flag = 1 → import-index (unsigned 32-bit)
 *           flag = 2 → (intf-id:16 | method-slot:16)
 */
typedef enum {
    FORMAT_IR = -1,
    FORMAT_DATA,
    FORMAT_Op,
    FORMAT_CALL,
    FORMAT_JMP,
    FORMAT_WIDE,
    FORMAT_NEW,

    FORMAT_RTagImm,             // build_intern

    FORMAT_R_TI_Imm12,          // load_int_imm
    FORMAT_TI_Imm2,             // ret_int_imm
    FORMAT_RR_TI_MODE,          // int_cast

    /* single register */
    FORMAT_Rx,                  // 12-bit reg
    FORMAT_RxTag,               // 12-bit reg + (tag)imm8
    FORMAT_RImm2,               // 8-bit reg + imm16
    FORMAT_ROff2,               // 8-bit reg + off16
    FORMAT_RIdx2,               // 8-bit reg + index16
    FORMAT_RImmOff,             // 8-bit reg + imm8 + off8
    FORMAT_RxIdx12,             // 12-bit reg + index12

    /* two registers */
    FORMAT_RxRx,                // 12-bit reg + 12-bit reg
    FORMAT_RRImm,               // 8-bit reg + 8-bit reg + imm8
    FORMAT_RROff,               // 8-bit reg + 8-bit reg + off8

    /* three registers */
    FORMAT_RRR,                 // 8-bit reg + 8-bit reg + 8-bit reg

    /* no registers */
    FORMAT_Tag,                 // 8-bit tag immediate
    FORMAT_Imm2,                // 16-bit immediate
    FORMAT_Idx2,                // 16-bit index
} OpFormat;

// clang-format on

extern char *__op_names[];
static inline char *op_name(OpCode code) { return __op_names[code]; }

extern OpFormat __op_formats[];
static inline OpFormat op_format(OpCode code) { return __op_formats[code]; }

extern char *tag_mapping[];
void bytecode_print(uint8_t *code, size_t start, size_t count);

extern char *intern_tag_name[];

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPCODE_H_ */
