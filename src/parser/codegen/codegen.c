/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <math.h>
#include "log.h"
#include "lowering.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline const char *format_name(int fmt)
{
    switch (fmt) {
        case FORMAT_Ax:
            return "Ax";
        case FORMAT_AxBx:
            return "AxBx";
        case FORMAT_ABC:
            return "ABC";
        case FORMAT_ABxx:
            return "ABxx";
        default:
            return "<?>";
    }
}

static void dump_lower_insn(KlrFunc *fn)
{
    char header[256];
    char line[256];

    // 1. 生成头部
    snprintf(header, sizeof(header),
             "====================== LIR(%s) ======================", fn->name);

    // 2. 生成等长尾部
    int len = strlen(header);
    memset(line, '=', len);
    line[len] = '\0';

    // 打印头部
    printf("%s\n", header);

    // 打印 LIR
    LowerInsn *insn;
    vector_foreach_ptr(insn, &fn->lir) {
        const char *fmt = format_name(insn->format);

        printf("[%04X]  %-20s  [%-4s]  ", insn->pc, opcode_name(insn->code), fmt);

        switch (insn->format) {
            case FORMAT_Ax:
                printf("Rd=%d", insn->rd);
                break;
            case FORMAT_AxBx:
                printf("Rd=%d Rs=%d", insn->rd, insn->rs);
                break;
            case FORMAT_ABC:
                printf("Rd=%d Rs=%d Rt=%d", insn->rd, insn->rs, insn->rt);
                break;
            case FORMAT_ABxx:
                printf("A=%d Offset=%d", insn->rd, insn->imm);
                break;
        }

        if (insn->target_bb) printf(" ->%%%s", klr_block_name(insn->target_bb));

        printf("\n");
    }

    // 打印尾部
    printf("%s\n", line);
}

static void klr_assign_vreg(KlrFunc *fn)
{
    int next_vreg = 0;

    /* 1. Assign vregs to function parameters (R0, R1...) */
    KlrParam *param;
    param_foreach(param, fn) { param->vreg = next_vreg++; }

    /* 2. Traverse all instructions in all basic blocks */
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            /* Ensure vreg is not already assigned */
            ASSERT(insn->vreg < 0);
            /* Assign vreg if the instruction defines a value (e.g., ADD, IR_LOCAL,
            GET_GLOBAL). We assume ir_has_value() checks if the OpCode produces a
            value
            */
            if (ir_has_value(insn)) {
                insn->vreg = next_vreg++;
            } else {
                /* For instructions that don't produce a value (e.g., STORE, JMP), we
                can set vreg to -1 or leave it unassigned */
                insn->vreg = -1;
            }
        }
    }

    /* Store the total count of vregs used for this function */
    fn->num_vregs = next_vreg;
}

/* Select the appropriate OP_CONST opcode for a given constant KlrValue.
 * out->rd must already be set by caller.
 */
static void select_const(KlrFunc *fn, KlrValue *v, LowerInsn *out)
{
    if (v->kind != KLR_VALUE_CONST) {
        UNREACHABLE();
        return;
    }

    KlrConst *kval = (KlrConst *)v;

    if (kval->which == CONST_NONE) {
        out->code = OP_CONST_NONE;
        out->format = FORMAT_Ax;
    } else if (kval->which == CONST_INT) {
        int64_t val = kval->ival;
        if (val == -1) {
            out->code = OP_CONST_INT_M1;
            out->format = FORMAT_Ax;
        } else if (val == 0) {
            out->code = OP_CONST_INT_0;
            out->format = FORMAT_Ax;
        } else if (val == 1) {
            out->code = OP_CONST_INT_1;
            out->format = FORMAT_Ax;
        } else if (val >= INT16_MIN && val <= INT16_MAX) {
            out->code = OP_CONST_INT_IMM;
            out->format = FORMAT_ABxx;
            out->imm = (int)val;
        } else {
            NYI();
        }
    } else if (kval->which == CONST_FLT) {
        double val = kval->fval;
        if (val == 0.0) {
            out->code = OP_CONST_FLOAT_0;
            out->format = FORMAT_Ax;
        } else if (val == -0.0) {
            out->code = OP_CONST_FLOAT_N0;
            out->format = FORMAT_Ax;
        } else if (isnan(val)) {
            out->code = OP_CONST_FLOAT_NAN;
            out->format = FORMAT_Ax;
        } else if (isinf(val) && val > 0) {
            out->code = OP_CONST_FLOAT_INF;
            out->format = FORMAT_Ax;
        } else if (isinf(val) && val < 0) {
            out->code = OP_CONST_FLOAT_NINF;
            out->format = FORMAT_Ax;
        } else {
            NYI();
        }
    } else if (kval->which == CONST_BOOL) {
        int val = kval->bval;
        if (val) {
            out->code = OP_CONST_TRUE;
        } else {
            out->code = OP_CONST_FALSE;
        }
        out->format = FORMAT_Ax;
    } else if (kval->which == CONST_STR) {
        NYI();
    } else {
        UNREACHABLE();
    }
}

/* Lower a single IR instruction into one LowerInsn. */
static void lower_one_insn(KlrFunc *fn, KlrInsn *insn)
{
    switch (insn->code) {
        case OP_MOVE: {
            KlrValue *dst = insn_oper_value(insn, 0);
            KlrValue *src = insn_oper_value(insn, 1);

            if (src->kind == KLR_VALUE_CONST) {
                KlrConst *kval = (KlrConst *)src;
                if (kval->which == CONST_INT) {
                    emit_const_int(dst->vreg, kval->ival, insn, fn);
                } else {
                    NYI();
                }
            } else {
                emit_AxBx(OP_MOVE, dst->vreg, src->vreg, insn, fn);
            }
            break;
        }

            // case OP_SET_GLOBAL: {
            //     KlrValue *src = insn_oper_value(insn, 0);
            //     int r = src->vreg;
            //     emit_Ax(OP_SET_GLOBAL, r, insn, fn);
            //     break;
            // }

        case OP_GET_GLOBAL: {
            KlrValue *dst = insn_oper_value(insn, 0);
            emit_Ax(OP_GET_GLOBAL, dst->vreg, insn, fn);
            break;
        }

        case OP_BINARY_ADD: {
            KlrValue *src1 = insn_oper_value(insn, 0);
            KlrValue *src2 = insn_oper_value(insn, 1);
            int dst = insn->vreg;
            int r1 = src1->vreg;
            int r2 = src2->vreg;

            int c1 = klr_is_const(src1);
            int c2 = klr_is_const(src2);

            if (!c1 && !c2) {
                emit_ABC(OP_BINARY_ADD, dst, r1, r2, insn, fn);
            } else if (!c1 && c2) {
                TypeSpec *ts = src1->ts;
                KlrConst *kval = (KlrConst *)src2;

                if (ts->kind == TYPE_INT) {
                    ASSERT(kval->which == CONST_INT);
                    int64_t val = kval->ival;
                    if (val >= INT8_MIN && val <= INT8_MAX) {
                        emit_ABC(OP_INT_ADD_IMM, dst, r1, val, insn, fn);
                    } else {
                        emit_const_int(dst, val, insn, fn);
                        emit_ABC(OP_INT_ADD, dst, dst, r1, insn, fn);
                    }
                } else {
                    NYI();
                }
            } else {
                NYI(); /* constant folding for ADD not implemented yet */
            }
            break;
        }

        case OP_BINARY_CMP_LT: {
            KlrValue *src1 = insn_oper_value(insn, 0);
            KlrValue *src2 = insn_oper_value(insn, 1);
            int dst = insn->vreg;
            int r1 = src1->vreg;
            int r2 = src2->vreg;

            int c1 = klr_is_const(src1);
            int c2 = klr_is_const(src2);

            /* reg < reg */
            if (!c1 && !c2) {
                emit_ABC(OP_BINARY_CMP_LT, dst, r1, r2, insn, fn);
                break;
            }

            /* reg < imm */
            if (!c1 && c2) {
                TypeSpec *ts = src1->ts;
                KlrConst *kval = (KlrConst *)src2;

                if (ts->kind == TYPE_INT) {
                    ASSERT(kval->which == CONST_INT);
                    emit_ABC(OP_INT_CMP_LT_IMM, dst, r1, (int)kval->ival, insn, fn);
                } else {
                    NYI();
                }

                break;
            }

            NYI(); /* imm < reg and imm < imm not supported yet */

            break;
        }

        case OP_IR_JMP_COND: {
            break;
        }

        case OP_RETURN: {
            ASSERT(insn->vreg == -1);
            KlrValue *src = insn_oper_value(insn, 0);
            int r = src->vreg;
            int c = klr_is_const(src);

            if (c) {
                KlrConst *kval = (KlrConst *)src;
                ASSERT(kval->which == CONST_INT);
                emit_const_int(0, kval->ival, insn, fn);
            } else {
                emit_Ax(OP_RETURN, r, insn, fn);
            }
            break;
        }

        case OP_RETURN_NONE: {
            emit_Ax(OP_RETURN_NONE, 0, insn, fn);
            break;
        }

        default: {
            fprintf(stderr, "lowering: unhandled IR opcode `%s`\n",
                    opcode_name(insn->code));
            abort();
            break;
        }
    }
}

/*
 * Lower SSA IR into a linear LIR stream (Vector<LowerInsn>).
 *
 * Responsibilities:
 *   - Instruction selection (ISel)
 *   - Constant folding into immediate forms
 *   - Pattern fusion (e.g., CMP + JMP_COND)
 *   - Linearization (assigning sequential PC)
 *   - Recording per-basic-block start_pc for later jump patching
 *
 * Output:
 *   fn->lir : Vector<LowerInsn>
 */
static void klr_lower_to_lir(KlrFunc *fn)
{
    vector_init(&fn->lir, sizeof(LowerInsn));

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        /* First LIR index for this basic block */
        bb->start_pc = vector_size(&fn->lir);

        KlrInsn *insn;
        insn_foreach(insn, bb) {
            /* Instruction Selection & Splitting */
            lower_one_insn(fn, insn);
        }
    }
}

void kl_do_lowering(ParserState *ps)
{
    KlrModule *m = ps->module;
    Vector *fns = &m->functions;
    KlrFunc *fn;
    vector_foreach(fn, fns) {
        log_info("Lowering function: %s", fn->name);
        klr_assign_vreg(fn);
        klr_lower_to_lir(fn);
        dump_lower_insn(fn);
    }
}

#ifdef __cplusplus
}
#endif
