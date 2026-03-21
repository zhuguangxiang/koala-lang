/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "encode.h"
#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------
 *  Encoding helpers
 *-----------------------------------------*/
static inline uint32_t encode_Ax(int op, int rd)
{
    return ((uint32_t)op << 24) | ((uint32_t)rd << 12);
}

static inline uint32_t encode_AxBx(int op, int rd, int rs)
{
    return ((uint32_t)op << 24) | ((uint32_t)rd << 12) | (uint32_t)rs;
}

static inline uint32_t encode_ABC(int op, int rd, int rs, int rt)
{
    return ((uint32_t)op << 24) | ((uint32_t)rd << 16) | ((uint32_t)rs << 8) |
           (uint32_t)rt;
}

static inline uint32_t encode_ABxx(int op, int rd, int imm16)
{
    return ((uint32_t)op << 24) | ((uint32_t)rd << 16) |
           ((uint32_t)(imm16 & 0xFFFF));
}

/*-----------------------------------------
 *  Unified encoder
 *-----------------------------------------*/
uint32_t encode_insn(LowerInsn *insn)
{
    switch (op_format(insn->code)) {
        case FORMAT_Ax:
            return encode_Ax(insn->code, insn->rd);

        case FORMAT_AxBx:
            return encode_AxBx(insn->code, insn->rd, insn->rs);

        case FORMAT_ABC:
            return encode_ABC(insn->code, insn->rd, insn->rs, insn->rt);

        case FORMAT_ABxx:
            return encode_ABxx(insn->code, insn->rd, insn->imm);

        default:
            fprintf(stderr, "encode_insn: unknown format for %s\n",
                    op_name(insn->code));
            abort();
    }
}

/*-----------------------------------------
 *  Unified decoder (32-bit → LowerInsn)
 *-----------------------------------------*/
void decode_insn(uint32_t raw, LowerInsn *out)
{
    out->code = (raw >> 24) & 0xFF;

    switch (op_format(out->code)) {
        case FORMAT_Ax:
            /* [op:8][A:12][0:12] */
            out->rd = (raw >> 12) & 0xFFF;
            out->rs = 0;
            out->rt = 0;
            out->imm = 0;
            break;

        case FORMAT_AxBx:
            /* [op:8][Ax:12][Bx:12] */
            out->rd = (raw >> 12) & 0xFFF;
            out->rs = raw & 0xFFF;
            out->rt = 0;
            out->imm = 0;
            break;

        case FORMAT_ABC:
            /* [op:8][A:8][B:8][C:8] */
            out->rd = (raw >> 16) & 0xFF;
            out->rs = (raw >> 8) & 0xFF;
            out->rt = raw & 0xFF;
            out->imm = 0;
            break;

        case FORMAT_ABxx:
            /* [op:8][A:8][Offset:16] */
            out->rd = (raw >> 16) & 0xFF;
            out->rs = 0;
            out->rt = 0;
            out->imm = raw & 0xFFFF;
            break;

        default:
            fprintf(stderr, "decode_insn: unknown format for opcode %d\n",
                    out->code);
            abort();
    }

    out->target_bb = NULL;
    out->origin = NULL;
}

#ifdef __cplusplus
}
#endif
