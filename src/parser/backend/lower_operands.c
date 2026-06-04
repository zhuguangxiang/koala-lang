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

static inline int fits_in_imm8(int64_t x) { return x >= INT8_MIN && x <= INT8_MAX; }
static inline int fits_in_imm12(int64_t x) { return x >= INT12_MIN && x <= INT12_MAX; }
static inline int fits_in_imm16(int64_t x) { return x >= INT16_MIN && x <= INT16_MAX; }
static inline int fits_in_uimm8(uint64_t x) { return x <= UINT8_MAX; }
static inline int fits_in_uimm12(uint64_t x) { return x <= UINT12_MAX; }
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

static void check_in_imm12(KlrConst *kc)
{
    if (kc->which == CONST_INT) {
        if (!fits_in_imm12(kc->ival)) {
            panic("Constant %ld does not fit in 12-bit immediate", kc->ival);
        }
    } else {
        ASSERT(kc->which == CONST_UINT);
        if (!fits_in_uimm12(kc->ival)) {
            panic("Constant %lu does not fit in 12-bit unsigned immediate", kc->ival);
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

static int get_type_info(KlrConst *kc)
{
    int type_info = 0;
    switch (kc->which) {
        case CONST_INT: {
            type_info |= 1 << 3;
            type_info |= 0 << 2;
            type_info |= __builtin_ctz(kc->len);
            break;
        }
        case CONST_UINT: {
            type_info |= 1 << 3;
            type_info |= 1 << 2;
            type_info |= __builtin_ctz(kc->len);
            break;
        }
        default: {
            NYI();
            break;
        }
    }
    return type_info;
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

static void lower_binary_opers(KlrInsn *insn)
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

static void lower_call_opers(KlrInsn *insn)
{
    KlrValue *fn_val = insn_oper_value(insn, 0);
    ASSERT(klr_is_func(fn_val) || klr_is_extfunc(fn_val) || klr_is_intf(fn_val) ||
           klr_is_ext_intf(fn_val));

    set_raw_reg(&insn->raws[0], insn->vreg);
    set_raw_imm(&insn->raws[1], insn->num_args);
    set_raw_func(&insn->raws[2], fn_val);
}

static void lower_move_opers(KlrInsn *insn, KlMachModule *m)
{
    switch (insn->code) {
        case OP_MOVE: {
            KlrValue *dst = insn_oper_value(insn, 0);
            KlrValue *src = insn_oper_value(insn, 1);

            ASSERT(klr_is_local(dst) || klr_is_param(dst));
            ASSERT(!klr_is_const(src));

            /* move reg, reg */
            set_raw_reg(&insn->raws[0], dst->vreg);
            set_raw_reg(&insn->raws[1], src->vreg);
            break;
        }

        case OP_LOAD_INT_IMM:
        case OP_LOAD_UINT_IMM: {
            KlrValue *dst = insn_oper_value(insn, 0);
            KlrValue *imm_val = insn_oper_value(insn, 1);

            ASSERT(klr_is_local(dst) || klr_is_param(dst));
            ASSERT(klr_is_const(imm_val));

            KlrConst *kc = (KlrConst *)imm_val;
            check_in_imm12(kc);
            int64_t imm = kc->ival;

            /* load reg, imm */
            set_raw_reg(&insn->raws[0], dst->vreg);
            int ti = get_type_info(kc);
            set_raw_imm(&insn->raws[1], ti);
            set_raw_imm(&insn->raws[2], imm);
            break;
        }

        case OP_LOAD_TAG: {
            KlrValue *dst = insn_oper_value(insn, 0);
            KlrValue *tag = insn_oper_value(insn, 1);

            ASSERT(klr_is_local(dst) || klr_is_param(dst));
            ASSERT(klr_is_const(tag));

            KlrConst *kc = (KlrConst *)tag;

            /* load reg, tag */
            set_raw_reg(&insn->raws[0], dst->vreg);
            set_raw_imm(&insn->raws[1], kc->spec_tag);
            break;
        }

        case OP_LOADK: {
            KlrValue *dst = insn_oper_value(insn, 0);
            KlrValue *cst = insn_oper_value(insn, 1);

            ASSERT(klr_is_local(dst) || klr_is_param(dst));
            ASSERT(klr_is_const(cst));

            KlrConst *kc = (KlrConst *)cst;

            /* const_load reg, const_index */
            set_raw_reg(&insn->raws[0], dst->vreg);
            KlMachConst *entry = kl_mach_add_const(kc, m);
            set_raw_const(&insn->raws[1], entry->index);
            break;
        }

        default: {
            UNREACHABLE();
            break;
        }
    }
}

static void lower_ret_opers(KlrInsn *insn, KlMachModule *m)
{
    switch (insn->code) {
        case OP_RET: {
            KlrValue *ret = insn_oper_value(insn, 0);
            ASSERT(!klr_is_const(ret));
            /* ret reg */
            set_raw_reg(&insn->raws[0], ret->vreg);
            break;
        }

        case OP_RET_INT_IMM:
        case OP_RET_UINT_IMM: {
            KlrValue *ret = insn_oper_value(insn, 0);
            ASSERT(klr_is_const(ret));
            KlrConst *kc = (KlrConst *)ret;
            check_in_imm16(kc);
            int64_t imm = kc->ival;
            /* ret imm */
            int type_info = get_type_info(kc);
            set_raw_imm(&insn->raws[0], type_info);
            set_raw_imm(&insn->raws[1], imm);
            break;
        }

        case OP_RET_TAG: {
            KlrValue *ret = insn_oper_value(insn, 0);
            ASSERT(klr_is_const(ret));
            KlrConst *kc = (KlrConst *)ret;
            /* ret tag */
            set_raw_imm(&insn->raws[0], kc->spec_tag);
            break;
        }

        case OP_RET_CONST: {
            KlrValue *ret = insn_oper_value(insn, 0);
            ASSERT(klr_is_const(ret));
            KlrConst *kc = (KlrConst *)ret;
            KlMachConst *entry = kl_mach_add_const(kc, m);
            set_raw_const(&insn->raws[0], entry->index);
            break;
        }

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

static void lower_jmp_opers(KlrInsn *insn)
{
    KlrValue *target = insn_oper_value(insn, 0);
    ASSERT(klr_is_block(target));

    /* jmp target_bb */
    set_raw_block(&insn->raws[0], (KlrBasicBlock *)target);
}

static inline int is_binary(OpCode op)
{
    return (op >= OP_INT_ADD && op <= OP_INT_CMPGE_IMM) ||
           (op >= OP_UINT_ADD_IMM && op <= OP_UINT_CMPGE_IMM) ||
           (op >= OP_FLOAT_ADD && op <= OP_FLOAT_CMPGE) || (op >= OP_LAND && op <= OP_LOR);
}

static inline int is_move(OpCode op) { return op >= OP_MOVE && op <= OP_LOADK; }
static inline int is_return(OpCode op) { return op >= OP_RET && op <= OP_RET_VOID; }

static void lower_logic_not_opers(KlrInsn *insn)
{
    KlrValue *val = insn_oper_value(insn, 0);
    ASSERT(klr_is_insn(val) || klr_is_param(val) || klr_is_local(val));

    /* lnot reg */
    set_raw_reg(&insn->raws[0], insn->vreg);
    set_raw_reg(&insn->raws[1], val->vreg);
}

static void lower_ir_cast_opers(KlrInsn *insn)
{
    KlrValue *src = insn_oper_value(insn, 0);
    set_raw_imm(&insn->raws[0], insn->vreg);
    set_raw_imm(&insn->raws[1], src->vreg);
}

static void lower_int_cast_opers(KlrInsn *insn)
{
    KlrValue *src = insn_oper_value(insn, 0);
    set_raw_imm(&insn->raws[0], insn->vreg);
    set_raw_imm(&insn->raws[1], src->vreg);
    set_raw_imm(&insn->raws[2], insn->cast_flag);
}

static void lower_float_cast_opers(KlrInsn *insn)
{
    KlrValue *src = insn_oper_value(insn, 0);
    set_raw_imm(&insn->raws[0], insn->vreg);
    set_raw_imm(&insn->raws[1], src->vreg);
    set_raw_imm(&insn->raws[2], insn->cast_flag);
}

static void lower_ref_eq_ne_null_opers(KlrInsn *insn)
{
    KlrValue *src = insn_oper_value(insn, 0);
    set_raw_imm(&insn->raws[0], insn->vreg);
    set_raw_imm(&insn->raws[1], src->vreg);
}

static void lower_build_intern_opers(KlrInsn *insn)
{
    set_raw_reg(&insn->raws[0], insn->vreg);
    set_raw_imm(&insn->raws[1], insn->intern_tag);
    set_raw_imm(&insn->raws[2], insn->num_args);
}

static void lower_new_opers(KlrInsn *insn, KlMachModule *m)
{
    set_raw_reg(&insn->raws[0], insn->vreg);

    KlrValue *val = insn_oper_value(insn, 0);

    if (val->kind == KLR_VALUE_KLASS) {
        KlrKlass *kls = (KlrKlass *)val;
        set_raw_imm(&insn->raws[1], kls->index);
    } else if (val->kind == KLR_VALUE_EXT_KLASS) {
        KlrExtKlass *kls = (KlrExtKlass *)val;
        KlrExtModule *mod = kls->module;
        int index = mach_import_add_klass(m, mod->name, kls->name);
        set_raw_imm(&insn->raws[1], index);
    } else {
        UNREACHABLE();
    }
}

static void lower_set_field_opers(KlrInsn *insn, KlMachModule *m)
{
    KlrValue *obj = insn_oper_value(insn, 0);
    KlrValue *src = insn_oper_value(insn, 2);
    set_raw_reg(&insn->raws[0], obj->vreg);
    set_raw_reg(&insn->raws[1], src->vreg);

    KlrValue *val = insn_oper_value(insn, 1);
    ASSERT(val->kind == KLR_VALUE_FIELD);
    KlrField *fld = (KlrField *)val;
    set_raw_imm(&insn->raws[2], fld->index);
}

static void lower_get_field_opers(KlrInsn *insn, KlMachModule *m)
{
    KlrValue *obj = insn_oper_value(insn, 0);
    set_raw_reg(&insn->raws[0], insn->vreg);
    set_raw_reg(&insn->raws[1], obj->vreg);

    KlrValue *val = insn_oper_value(insn, 1);
    ASSERT(val->kind == KLR_VALUE_FIELD);
    KlrField *fld = (KlrField *)val;
    set_raw_imm(&insn->raws[2], fld->index);
}

static void lower_set_field_ext_opers(KlrInsn *insn, KlMachModule *m)
{
    KlrValue *obj = insn_oper_value(insn, 0);
    KlrValue *src = insn_oper_value(insn, 2);
    set_raw_reg(&insn->raws[0], obj->vreg);
    set_raw_reg(&insn->raws[1], src->vreg);

    KlrValue *val = insn_oper_value(insn, 1);
    ASSERT(val->kind == KLR_VALUE_EXT_FIELD);
    KlrExtField *fld = (KlrExtField *)val;
    KlrExtKlass *kls = fld->klass;
    KlrExtModule *mod = kls->module;

    int index = mach_import_add_field(m, mod->name, kls->name, fld->name);
    set_raw_imm(&insn->raws[2], index);
}

static void lower_get_field_ext_opers(KlrInsn *insn, KlMachModule *m)
{
    KlrValue *obj = insn_oper_value(insn, 0);
    set_raw_reg(&insn->raws[0], insn->vreg);
    set_raw_reg(&insn->raws[1], obj->vreg);

    KlrValue *val = insn_oper_value(insn, 1);
    ASSERT(val->kind == KLR_VALUE_EXT_FIELD);
    KlrExtField *fld = (KlrExtField *)val;
    KlrExtKlass *kls = fld->klass;
    KlrExtModule *mod = kls->module;

    int index = mach_import_add_field(m, mod->name, kls->name, fld->name);
    set_raw_imm(&insn->raws[2], index);
}

static void lower_move_true_opers(KlrInsn *insn, KlMachModule *m)
{
    KlrValue *var = insn_oper_value(insn, 0);
    KlrValue *cond = insn_oper_value(insn, 1);
    KlrValue *val = insn_oper_value(insn, 2);

    set_raw_reg(&insn->raws[0], var->vreg);
    set_raw_reg(&insn->raws[1], cond->vreg);
    set_raw_reg(&insn->raws[2], val->vreg);
}

static void lower_seq_get_opers(KlrInsn *insn, KlMachModule *m)
{
    KlrValue *obj = insn_oper_value(insn, 0);
    KlrValue *index = insn_oper_value(insn, 1);
    set_raw_reg(&insn->raws[0], insn->vreg);
    set_raw_reg(&insn->raws[1], obj->vreg);
    if (klr_is_const(index)) {
        KlrConst *kc = (KlrConst *)index;
        check_in_imm8(kc);
        int64_t imm = kc->ival;
        set_raw_imm(&insn->raws[2], imm);
    } else {
        set_raw_reg(&insn->raws[2], index->vreg);
    }
}

static void lower_seq_set_opers(KlrInsn *insn, KlMachModule *m)
{
    KlrValue *obj = insn_oper_value(insn, 0);
    KlrValue *index = insn_oper_value(insn, 1);
    KlrValue *val = insn_oper_value(insn, 2);

    set_raw_reg(&insn->raws[0], obj->vreg);
    set_raw_reg(&insn->raws[1], val->vreg);

    if (klr_is_const(index)) {
        KlrConst *kc = (KlrConst *)index;
        check_in_imm8(kc);
        int64_t imm = kc->ival;
        set_raw_imm(&insn->raws[2], imm);
    } else {
        set_raw_reg(&insn->raws[2], index->vreg);
    }
}

static void lower_seq_len_opers(KlrInsn *insn, KlMachModule *m)
{
    KlrValue *obj = insn_oper_value(insn, 0);
    set_raw_reg(&insn->raws[0], insn->vreg);
    set_raw_reg(&insn->raws[1], obj->vreg);
}

static void lower_make_intf_opers(KlrInsn *insn, KlMachModule *m)
{
    KlrValue *obj = insn_oper_value(insn, 0);
    set_raw_reg(&insn->raws[0], insn->vreg);
    set_raw_reg(&insn->raws[1], obj->vreg);
    // insn->raws[2] is already set in irgen when creating OP_MAKE_INTF,
    // so we don't need to set it here.
}

void kl_lower_operands(KlrFunc *fn, KlMachModule *m)
{
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (is_binary(insn->code)) {
                lower_binary_opers(insn);
                continue;
            }

            if (is_move(insn->code)) {
                lower_move_opers(insn, m);
                continue;
            }

            if (is_return(insn->code)) {
                lower_ret_opers(insn, m);
                continue;
            }

            switch (insn->code) {
                case OP_CALL:
                case OP_TAIL_CALL: {
                    lower_call_opers(insn);
                    break;
                }

                case OP_JMP: {
                    lower_jmp_opers(insn);
                    break;
                }

                case OP_LNOT: {
                    lower_logic_not_opers(insn);
                    break;
                }

                case OP_INT_CAST: {
                    lower_int_cast_opers(insn);
                    break;
                }

                case OP_FLOAT_CAST: {
                    lower_float_cast_opers(insn);
                    break;
                }

                case OP_IR_CAST: {
                    lower_ir_cast_opers(insn);
                    break;
                }

                case OP_REF_EQ_NULL:
                case OP_REF_NE_NULL: {
                    lower_ref_eq_ne_null_opers(insn);
                    break;
                }

                case OP_BUILD_INTERN: {
                    lower_build_intern_opers(insn);
                    break;
                }

                case OP_NEW:
                case OP_NEW_EXT: {
                    lower_new_opers(insn, m);
                    break;
                }

                case OP_SET_FIELD: {
                    lower_set_field_opers(insn, m);
                    break;
                }

                case OP_GET_FIELD: {
                    lower_get_field_opers(insn, m);
                    break;
                }

                case OP_SET_FIELD_EXT: {
                    lower_set_field_ext_opers(insn, m);
                    break;
                }

                case OP_GET_FIELD_EXT: {
                    lower_get_field_ext_opers(insn, m);
                    break;
                }

                case OP_MOVE_TRUE: {
                    lower_move_true_opers(insn, m);
                    break;
                }

                case OP_SEQ_GET:
                case OP_SEQ_GET_IMM: {
                    lower_seq_get_opers(insn, m);
                    break;
                }

                case OP_SEQ_LEN: {
                    lower_seq_len_opers(insn, m);
                    break;
                }

                case OP_SEQ_SET_IMM:
                case OP_SEQ_SET: {
                    lower_seq_set_opers(insn, m);
                    break;
                }

                case OP_MAKE_INTF:
                case OP_UPCAST_INTF: {
                    lower_make_intf_opers(insn, m);
                    break;
                }

                case OP_IR_LOCAL:
                case OP_IR_JMP_COND: {
                    // fall-through, backend will handle these IR-specific
                    // instructions with special patterns, so we don't lower
                    // them here.
                    break;
                }

                // other patterns...
                default: {
                    break;
                }
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
