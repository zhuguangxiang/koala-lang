/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cgen.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

const void dump_value(char *name, int value, int is_last)
{
    const int FIELD_WIDTH = 10;
    char buf[64];

    if (name) {
        int len = snprintf(buf, sizeof(buf), "%s = %d", name, value);
        int target = is_last ? FIELD_WIDTH - 1 : FIELD_WIDTH;
        int pad = (len >= target) ? 1 : (target - len);
        memset(buf + len, ' ', pad);
        buf[len + pad] = '\0';
        fputs(buf, stdout);
    } else {
        int target = is_last ? FIELD_WIDTH - 1 : FIELD_WIDTH;
        for (int i = 0; i < target; i++) putchar(' ');
    }
}

static void dump_mach_insn(KlMachInsn *mi)
{
    printf("%4d  %-12s  ", mi->pc, op_name(mi->code));

    switch (mi->format) {
        case FORMAT_Ax:
            dump_value("Ax", mi->Ax, 0);
            dump_value(NULL, 0, 0);
            dump_value(NULL, 0, 1);
            break;

        case FORMAT_Axx:
            dump_value("Axx", mi->Axx, 0);
            dump_value(NULL, 0, 0);
            dump_value(NULL, 0, 1);
            break;

        case FORMAT_ABC:
            dump_value("A", mi->A, 0);
            dump_value("B", mi->B, 0);
            dump_value("C", mi->C, 1);
            break;

        case FORMAT_AxBx:
            dump_value("Ax", mi->Ax, 0);
            dump_value("Bx", mi->Bx, 0);
            dump_value(NULL, 0, 1);
            break;

        case FORMAT_ABxx:
            dump_value("A", mi->A, 0);
            dump_value("Bxx", mi->Bxx, 0);
            dump_value(NULL, 0, 1);
            break;

        case FORMAT_Op:
            // no operand
            break;

        default:
            printf("(unknown format)");
            break;
    }

    if (mi->target) {
        KlrBasicBlock *origin = mi->target->origin;
        printf(";; -> %%%s", klr_block_name(origin));
    }

    printf("\n");
}

static void dump_mach_block(KlMachBlock *mb)
{
    printf("%%%s:\n", klr_block_name(mb->origin));

    KlMachInsn *mi;
    vector_foreach(mi, &mb->insns) {
        dump_mach_insn(mi);
    }

    printf("\n");
}

void klm_dump_func(KlMachFunc *fn)
{
    printf("====== Linearization @%s ======\n\n", fn->origin->name);

    KlMachBlock *mb;
    list_foreach(mb, link, &fn->bb_list) {
        dump_mach_block(mb);
    }

    printf("=== End of Linearization @%s ====\n\n", fn->origin->name);
}

/* Extract integer value from a raw operand. */
static inline int get_raw_value(KlrInsn *insn, KlrRawOper *op)
{
    switch (op->kind) {
        case RAW_OPER_REG: {
            /* Use the instruction's result vreg. */
            return insn->vreg;
        }

        case RAW_OPER_INDEX: {
            /* Use vreg from an input operand. */
            KlrValue *val = insn_oper_value(insn, op->index);
            return val->vreg;
        }

        case RAW_OPER_IMM: {
            return op->imm;
        }

        case RAW_OPER_CONST: {
            NYI();
            /* Constant pool index should be assigned before emit. */
            return op->kval->index;
        }

        case RAW_OPER_GLOBAL: {
            NYI();
            return op->global_index;
        }

        default: {
            UNREACHABLE();
            return 0;
        }
    }
}

void dump_emitted_code(const Buffer *code)
{
    const uint32_t *p = (uint32_t *)code->buf;
    size_t len = code->len / 4;

    for (size_t pc = 0; pc < len; pc += 1) {
        uint32_t insn = *(p + pc);

        uint32_t opcode = (insn >> 24) & 0xFFu;
        uint32_t a = (insn >> 16) & 0xFFu;
        uint32_t b = (insn >> 8) & 0xFFu;
        uint32_t c = (insn >> 0) & 0xFFu;

        printf("%04zu:  %08X   op=%02X  A=%02X  B=%02X  C=%02X\n", pc, insn,
               opcode, a, b, c);
    }
}

static void emit_mach_insn(KlMachInsn *mi, Buffer *buf)
{
    int32_t bytecode = 0;

    KlrInsn *insn = mi->origin;
    KlrRawOper *op0 = &mi->opers[0];
    KlrRawOper *op1 = &mi->opers[1];
    KlrRawOper *op2 = &mi->opers[2];

    switch (mi->format) {
        case FORMAT_Op: {
            // | op:8 | ----:24 |
            bytecode |= (mi->code & 0xFFu) << 24;
            break;
        }

        case FORMAT_Ax: {
            // | op:8 | ----:12 | Ax:12 |
            uint32_t A = get_raw_value(insn, op0);
            bytecode |= (mi->code & 0xFFu) << 24;
            bytecode |= (A & 0xFFFu);
            break;
        }

        case FORMAT_Axx: {
            // | op:8 | Axx:24 |
            uint32_t A = mi->Axx;
            bytecode |= (mi->code & 0xFFu) << 24;
            bytecode |= (A & 0xFFFFFFu);
            break;
        }

        case FORMAT_ABC: {
            // | op:8 | A:8 | B:8 | C:8 |
            uint32_t A = get_raw_value(insn, op0);
            uint32_t B = get_raw_value(insn, op1);
            uint32_t C = get_raw_value(insn, op2);
            bytecode |= (mi->code & 0xFFu) << 24;
            bytecode |= (A & 0xFFu) << 16;
            bytecode |= (B & 0xFFu) << 8;
            bytecode |= (C & 0xFFu);
            break;
        }

        case FORMAT_AxBx: {
            // | op:8 | Ax:12 | Bx:12 |
            uint32_t A = get_raw_value(insn, op0);
            uint32_t B = get_raw_value(insn, op1);
            bytecode |= (mi->code & 0xFFu) << 24;
            bytecode |= (A & 0xFFFu) << 12;
            bytecode |= (B & 0xFFFu);
            break;
        }

        case FORMAT_ABxx: {
            // | op:8 | A:8 | Bxx:16 |
            uint32_t A = mi->A;
            uint32_t Bxx = mi->Bxx;
            bytecode |= (mi->code & 0xFFu) << 24;
            bytecode |= (A & 0xFFu) << 16;
            bytecode |= (Bxx & 0xFFFFu);
            break;
        }

        default: {
            UNREACHABLE();
            break;
        }
    }

    buf_write_uint32(buf, bytecode);
}

static void klr_emit_func(KlMachFunc *mfn, Buffer *buf)
{
    KlMachBlock *mb;
    list_foreach(mb, link, &mfn->bb_list) {
        KlMachInsn *mi;
        vector_foreach(mi, &mb->insns) {
            emit_mach_insn(mi, buf);
        }
    }
}

/* Extract basic block target from a raw operand. */
static inline KlMachBlock *get_raw_block(KlrRawOper *op)
{
    if (op->kind != RAW_OPER_BLOCK) return NULL;

    KlrBasicBlock *bb = op->bb;
    return (KlMachBlock *)bb->mach; /* mapped during linearization */
}

/* Populate machine instruction fields from raw operands.
 * isel must fully define raw_opers[]; this function only
 * maps them into physical encoding fields based on format.
 */
static void fill_mach_insn(KlMachInsn *mi, KlrInsn *insn)
{
    mi->code = insn->code;
    mi->format = op_format(insn->code);
    /* pc is assigned by linearization pass. */
    mi->pc = -1;
    mi->origin = insn;

    mi->opers[0] = insn->raw_opers[0];
    mi->opers[1] = insn->raw_opers[1];
    mi->opers[2] = insn->raw_opers[2];

    if (insn->code == OP_JMP) {
        /* For unconditional jump, operand is a single basic block. */
        KlrValue *bb = insn_oper_value(insn, 0);
        mi->target = ((KlrBasicBlock *)bb)->mach;
        return;
    }

    if (insn->code == OP_CALL) {
        /* For call, operand 0 is func, operands 1..n are args. */
        KlrValue *fn = insn_oper_value(insn, 0);
        ASSERT(klr_is_func(fn));
        if (ir_has_value(insn)) {
            mi->A = insn->vreg;
        } else {
            mi->A = -1;
        }
        mi->B = insn->num_args;
        mi->C = 0; // offset
        return;
    }

    KlrRawOper *op0 = &insn->raw_opers[0];
    KlrRawOper *op1 = &insn->raw_opers[1];
    KlrRawOper *op2 = &insn->raw_opers[2];

    /* Map raw operands into physical encoding fields. */
    switch (mi->format) {
        case FORMAT_Ax: {
            mi->Ax = get_raw_value(insn, op0);
            break;
        }

        case FORMAT_Axx: {
            mi->Axx = get_raw_value(insn, op0);
            break;
        }

        case FORMAT_ABC: {
            mi->A = get_raw_value(insn, op0);
            mi->B = get_raw_value(insn, op1);
            mi->C = get_raw_value(insn, op2);
            break;
        }

        case FORMAT_AxBx: {
            mi->Ax = get_raw_value(insn, op0);
            mi->Bx = get_raw_value(insn, op1);
            break;
        }

        case FORMAT_ABxx: {
            mi->A = get_raw_value(insn, op0);
            mi->Bxx = get_raw_value(insn, op1);
            break;
        }

        default: {
            /* Unreachable for valid opcodes. */
            break;
        }
    }
}

KlMachFunc *klm_linearize_func(KlrFunc *fn)
{
    KlMachFunc *mfn = mm_alloc_obj(mfn);
    mfn->origin = fn;
    init_list(&mfn->bb_list);

    // build MachBlock
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlMachBlock *mb = mm_alloc_obj(mb);
        mb->origin = bb;
        bb->mach = mb;
        init_list(&mb->link);
        vector_init_ptr(&mb->insns);
        list_push_back(&mfn->bb_list, &mb->link);
    }

    // fill MachInsn
    KlMachBlock *mb;
    list_foreach(mb, link, &mfn->bb_list) {
        KlrBasicBlock *bb = mb->origin;

        // next block is fallthrough (may be NULL)
        KlMachBlock *next = list_next(mb, link, &mfn->bb_list);

        KlrInsn *insn;
        insn_foreach(insn, bb) {
            // OP_IR_LOCAL is pseudo instruction, not emitted as machine code,
            // so skip it.
            if (klr_is_local((KlrValue *)insn)) continue;

            // Lower OP_IR_JMP_COND into concrete machine-level jumps.
            // This pseudo instruction cannot be assigned a PC or encoded
            // directly. We must eliminate it here based solely on block layout
            // (fallthrough).
            //
            //   br cond, bb_true, bb_false
            //
            // If bb_true is the fallthrough block, we invert the condition:
            //
            //   if (!cond) goto bb_false
            //
            // Otherwise:
            //
            //   if (cond) goto bb_true
            //   goto bb_false
            //
            // After this lowering, the instruction stream contains only real,
            // PC-carrying jump instructions, allowing patch_pc() and
            // patch_branches() to compute correct offsets.
            if (insn->code == OP_IR_JMP_COND) {
                KlrValue *cond = insn_oper_value(insn, 0);

                KlrValue *val = insn_oper_value(insn, 1);
                ASSERT(klr_is_block(val));
                KlrBasicBlock *bb_true = (KlrBasicBlock *)val;

                val = insn_oper_value(insn, 2);
                ASSERT(klr_is_block(val));
                KlrBasicBlock *bb_false = (KlrBasicBlock *)val;

                int fallthrough = (next && next->origin == bb_true);

                if (fallthrough) {
                    // if (!cond) goto false
                    KlMachInsn *mi = mm_alloc_obj(mi);
                    mi->code = OP_JMP_FALSE;
                    mi->format = FORMAT_ABxx;
                    mi->origin = insn;
                    mi->A = cond->vreg;
                    mi->target = bb_false->mach;
                    vector_push_back(&mb->insns, &mi);
                } else {
                    // if (cond) goto true
                    KlMachInsn *mi1 = mm_alloc_obj(mi1);
                    mi1->code = OP_JMP_TRUE;
                    mi1->format = FORMAT_ABxx;
                    mi1->origin = insn;
                    mi1->A = cond->vreg;
                    mi1->target = bb_true->mach;
                    vector_push_back(&mb->insns, &mi1);

                    // goto false
                    KlMachInsn *mi2 = mm_alloc_obj(mi2);
                    mi2->code = OP_JMP;
                    mi2->format = FORMAT_Axx;
                    mi2->origin = insn;
                    mi2->target = bb_false->mach;
                    vector_push_back(&mb->insns, &mi2);
                }
                continue;
            }

            KlMachInsn *mi = mm_alloc_obj(mi);
            fill_mach_insn(mi, insn);
            vector_push_back(&mb->insns, &mi);
        }
    }

    // patch pc
    int pc = 0;

    list_foreach(mb, link, &mfn->bb_list) {
        // Record the starting PC of this block.
        mb->start_pc = pc;

        // Assign PC to each instruction in this block.
        KlMachInsn *mi;
        vector_foreach(mi, &mb->insns) {
            mi->pc = pc;
            pc++;
        }
    }

    // Optionally store total instruction count
    mfn->total_insns = pc;

    // patch jmp
    list_foreach(mb, link, &mfn->bb_list) {
        KlMachInsn *mi;
        vector_foreach(mi, &mb->insns) {
            if (mi->code == OP_JMP) {
                ASSERT(mi->format == FORMAT_Axx);
                ASSERT(mi->target);
                int target_pc = mi->target->start_pc;
                ASSERT(target_pc >= 0);
                /* Relative offset: target - (current + 1) */
                mi->Axx = target_pc - (mi->pc + 1);
            } else if (mi->code == OP_JMP_TRUE || mi->code == OP_JMP_FALSE) {
                ASSERT(mi->format == FORMAT_ABxx);
                ASSERT(mi->target);
                int target_pc = mi->target->start_pc;
                ASSERT(target_pc >= 0);
                /* Relative offset: target - (current + 1) */
                mi->Bxx = target_pc - (mi->pc + 1);
            } else {
                // do nothing for non-jump instructions
            }
        }
    }

    return mfn;
}

static int klr_do_cgen(KlrFunc *fn, void *data)
{
    log_info("cgen for func '%s'", fn->name);

    KlMachFunc *mfn = klm_linearize_func(fn);
    klm_dump_func(mfn);

    BUF(buf);
    klr_emit_func(mfn, &buf);
    dump_emitted_code(&buf);
    FINI_BUF(buf);

    return 0;
}

static KlrPass cgen_pass = {
    .name = "cgen-pass",
    .run = klr_do_cgen,
};

void build_cgen_pm(KlrPassManager *pm, int dump)
{
    pm_add_pass(pm, &cgen_pass, dump);
}

#ifdef __cplusplus
}
#endif
