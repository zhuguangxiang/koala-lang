/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cgen.h"
#include "ir.h"
#include "log.h"
#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void set_raw_reg(KlrRawOper *r, int vreg)
{
    r->kind = RAW_OPER_REG;
    r->vreg = vreg;
}

static inline void set_raw_imm(KlrRawOper *r, int imm)
{
    r->kind = RAW_OPER_IMM;
    r->imm = imm;
}

static inline void set_raw_const(KlrRawOper *r, int index)
{
    r->kind = RAW_OPER_CONST;
    r->index = index;
}

static inline void set_raw_func(KlrRawOper *r, KlrFunc *fn)
{
    r->kind = RAW_OPER_FUNC;
    r->ptr = fn;
}

static inline int fits_in_imm8(int64_t x) { return x >= INT8_MIN && x <= INT8_MAX; }
static inline int fits_in_imm16(int64_t x) { return x >= INT16_MIN && x <= INT16_MAX; }
static inline int fits_in_uimm8(uint64_t x) { return x <= UINT8_MAX; }
static inline int fits_in_uimm16(uint64_t x) { return x <= UINT16_MAX; }

static void check_in_imm8(KlrConst *kc)
{
    if (kc->which == CONST_INT) {
        if (!fits_in_imm8(kc->ival)) {
            panic("Constant %ld does not fit in 8-bit immediate", kc->ival);
        }
    } else {
        ASSERT(kc->which == CONST_UINT);
        if (!fits_in_uimm8(kc->ival)) {
            panic("Constant %lu does not fit in 8-bit unsigned immediate", kc->ival);
        }
    }
}

static void check_in_imm16(KlrConst *kc)
{
    if (kc->which == CONST_INT) {
        if (!fits_in_imm16(kc->ival)) {
            panic("Constant %ld does not fit in 16-bit immediate", kc->ival);
        }
    } else {
        ASSERT(kc->which == CONST_UINT);
        if (!fits_in_uimm16(kc->ival)) {
            panic("Constant %lu does not fit in 16-bit unsigned immediate", kc->ival);
        }
    }
}

static void lower_reg_reg(KlrInsn *insn)
{
    KlrValue *lhs = insn_oper_value(insn, 0);
    KlrValue *rhs = insn_oper_value(insn, 1);

    set_raw_reg(&insn->raws[0], insn->vreg);
    set_raw_reg(&insn->raws[1], lhs->vreg);
    set_raw_reg(&insn->raws[2], rhs->vreg);
}

static void lower_reg_imm(KlrInsn *insn)
{
    KlrValue *lhs = insn_oper_value(insn, 0);
    KlrValue *rhs = insn_oper_value(insn, 1);

    KlrConst *kc = (KlrConst *)rhs;
    check_in_imm8(kc);
    int64_t imm = kc->ival;

    set_raw_reg(&insn->raws[0], insn->vreg);
    set_raw_reg(&insn->raws[1], lhs->vreg);
    set_raw_imm(&insn->raws[2], imm);
}

static void lower_binary_opers(KlrInsn *insn, KlrFunc *fn)
{
    OpFormat fmt = op_format(insn->code);
    KlrValue *lhs = insn_oper_value(insn, 0);
    KlrValue *rhs = insn_oper_value(insn, 1);
    ASSERT(fmt == FORMAT_RRImm || fmt == FORMAT_RRR);

    int c = klr_is_const(rhs);

    if (!c) {
        /* reg op reg */
        lower_reg_reg(insn);
    } else {
        /* reg op imm */
        lower_reg_imm(insn);
    }
}

static void lower_call_opers(KlrInsn *insn, KlrFunc *fn)
{
    KlrValue *fn_val = insn_oper_value(insn, 0);
    ASSERT(klr_is_func(fn_val));

    set_raw_reg(&insn->raws[0], insn->vreg);
    set_raw_imm(&insn->raws[1], insn->num_args);
    set_raw_func(&insn->raws[2], (KlrFunc *)fn_val);
}

static void lower_ret_opers(KlrInsn *insn, KlrFunc *fn)
{
    switch (insn->code) {
        case OP_RET: {
            KlrValue *ret = insn_oper_value(insn, 0);
            ASSERT(!klr_is_const(ret));
            /* ret reg */
            set_raw_reg(&insn->raws[0], ret->vreg);
            break;
        }

            // case OP_RET_INT_IMM: {
            //     lower_ret_operands_ret_int_imm(insn, fn);
            //     break;
            // }

            // case OP_RET_VAL: {
            //     lower_ret_operands_ret_val(insn, fn);
            //     break;
            // }

            // case OP_RET_CONST: {
            //     lower_ret_operands_ret_const(insn, fn);
            //     break;
            // }

        case OP_RET_VOID: {
            /* no operands */
            ASSERT(insn->num_opers == 0);
            break;
        }

        default: {
            UNREACHABLE();
            break;
        }
    }
}

static void lower_move_opers(KlrInsn *insn, KlrFunc *fn)
{
    KlrValue *dst = insn_oper_value(insn, 0);
    KlrValue *src = insn_oper_value(insn, 1);

    ASSERT(klr_is_local(dst));
    ASSERT(!klr_is_const(src));

    /* move reg, reg */
    set_raw_reg(&insn->raws[0], dst->vreg);
    set_raw_reg(&insn->raws[1], src->vreg);
}

static void lower_move_int_imm_opers(KlrInsn *insn, KlrFunc *fn)
{
    KlrValue *dst = insn_oper_value(insn, 0);
    KlrValue *imm_val = insn_oper_value(insn, 1);

    ASSERT(klr_is_local(dst));
    ASSERT(klr_is_const(imm_val));

    KlrConst *kc = (KlrConst *)imm_val;
    check_in_imm16(kc);
    int64_t imm = kc->ival;

    /* move reg, imm */
    set_raw_reg(&insn->raws[0], dst->vreg);
    set_raw_imm(&insn->raws[1], imm);
}

static void lower_push_opers(KlrInsn *insn, KlrFunc *fn)
{
    KlrValue *src = insn_oper_value(insn, 0);

    switch (insn->code) {
        case OP_PUSH: {
            ASSERT(!klr_is_const(src));
            /* push reg */
            set_raw_reg(&insn->raws[0], src->vreg);
            break;
        }

        case OP_PUSH_INT_IMM: {
            ASSERT(klr_is_const(src));
            /* push imm */
            KlrConst *kc = (KlrConst *)src;
            check_in_imm16(kc);
            int64_t imm = kc->ival;
            set_raw_imm(&insn->raws[0], imm);
            break;
        }

            // case OP_PUSH_VAL:
            // case OP_PUSH_CONST: {
            //     /* push val/const */
            //     KlrValue *val = insn_oper_value(insn, 0);
            //     ASSERT(klr_is_const(val));
            //     set_raw_const(&insn->raws[0], ((KlrConst *)val)->index);
            //     break;
            // }

        default: {
            UNREACHABLE();
            break;
        }
    }
}

static void lower_const_load_opers(KlrInsn *insn, KlrFunc *fn, KlMachModule *ctx)
{
    KlrValue *dst = insn_oper_value(insn, 0);
    KlrValue *cst = insn_oper_value(insn, 1);

    ASSERT(klr_is_local(dst));
    ASSERT(klr_is_const(cst));

    KlrConst *kc = (KlrConst *)cst;

    /* const_load reg, const_index */
    set_raw_reg(&insn->raws[0], dst->vreg);

    int index = -1;

    switch (kc->which) {
        case CONST_INT: {
            index = kl_mach_const_add_int(ctx, kc->ival);
            break;
        }
        case CONST_UINT: {
            index = kl_mach_const_add_uint(ctx, kc->ival);
            break;
        }
        case CONST_FLT: {
            index = kl_mach_const_add_float(ctx, kc->fval);
            break;
        }
        case CONST_STR: {
            index = kl_mach_const_add_str(ctx, kc->sval);
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }

    set_raw_const(&insn->raws[1], index);
}

static inline int is_binary(OpCode op)
{
    return (op >= OP_INT_ADD && op <= OP_INT_CMPGE_IMM) ||
           (op >= OP_UINT_DIV && op <= OP_UINT_DIV_IMM) ||
           (op >= OP_FLOAT_ADD && op <= OP_FLOAT_CMPG);
}

static inline int is_return(OpCode op) { return op >= OP_RET && op <= OP_RET_VOID; }

static inline int is_push(OpCode op) { return op >= OP_PUSH && op <= OP_PUSH_CONST; }

void kl_lower_operands(KlrFunc *fn, KlMachModule *ctx)
{
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (is_binary(insn->code)) {
                lower_binary_opers(insn, fn);
                continue;
            }

            if (is_return(insn->code)) {
                lower_ret_opers(insn, fn);
                continue;
            }

            if (is_push(insn->code)) {
                lower_push_opers(insn, fn);
                continue;
            }

            switch (insn->code) {
                case OP_CALL: {
                    lower_call_opers(insn, fn);
                    break;
                }

                case OP_MOVE: {
                    lower_move_opers(insn, fn);
                    break;
                }

                case OP_CONST_INT_IMM: {
                    lower_move_int_imm_opers(insn, fn);
                    break;
                }

                case OP_CONST_LOAD: {
                    lower_const_load_opers(insn, fn, ctx);
                    break;
                }

                case OP_IR_LOCAL:
                case OP_IR_JMP_COND:
                case OP_JMP: {
                    // fall-through, backend will handle these IR-specific instructions
                    // with special patterns, so we don't lower them here.
                    break;
                }

                // other patterns...
                default: {
                    UNREACHABLE();
                    break;
                }
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
