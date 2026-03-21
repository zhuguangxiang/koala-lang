/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cgen.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

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

        case RAW_OPER_FUNC: {
            NYI();
            return op->func_index;
        }

        case RAW_OPER_NONE: {
            return 0; /* Unused operand slot. */
        }

        default: {
            UNREACHABLE();
            return 0;
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
    mi->format = opcode_format(insn->code);
    /* pc is assigned by linearization pass. */
    mi->pc = -1;
    mi->origin = insn;

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

        case FORMAT_Axxx: {
            mi->Axxx = get_raw_value(insn, op0);
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

    /* Branch targets from raw operands (if any). */
    mi->target_true = get_raw_block(op0);
    mi->target_false = get_raw_block(op1);
}

KlMachFunc *klm_linearize_func(KlrFunc *fn)
{
    KlMachFunc *mfn = mm_alloc_obj(mfn);
    mfn->origin = fn;
    init_list(&mfn->bb_list);

    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        KlMachBlock *mb = mm_alloc_obj(mb);
        mb->origin = bb;
        bb->mach = mb;
        init_list(&mb->link);
        vector_init_ptr(&mb->insns);
        list_push_back(&mfn->bb_list, &mb->link);
    }

    int pc = 0;

    KlMachBlock *mb;
    list_foreach(mb, link, &mfn->bb_list) {
        KlrBasicBlock *bb = mb->origin;

        mb->start_pc = pc;

        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (klr_is_local((KlrValue *)insn)) continue;

            KlMachInsn *mi = mm_alloc_obj(mi);
            fill_mach_insn(mi, insn);

            mi->pc = pc;
            pc++;

            vector_push_back(&mb->insns, &mi);
        }
    }

    return mfn;
}

static void dump_raw_operand(KlrInsn *insn, int slot, int value)
{
    KlrRawOper *op = &insn->raw_opers[slot];

    switch (op->kind) {
        case RAW_OPER_REG:
        case RAW_OPER_INDEX:
            printf("R%d", value);
            break;

        case RAW_OPER_IMM:
            printf("#%d", value);
            break;

        case RAW_OPER_CONST:
            printf("K%d", value);
            break;

        case RAW_OPER_BLOCK:
            printf("BB%d", value);
            break;

        case RAW_OPER_GLOBAL:
            printf("G%d", value);
            break;

        case RAW_OPER_FUNC:
            printf("F%d", value);
            break;

        default:
            printf("<?>%d", value);
            break;
    }
}

/**
 * Dump a single operand slot with aligned formatting.
 *
 * This function prints:
 *     <name>=<semantic_value><padding>
 *
 * The semantic value is derived from the original IR operand kind
 * (register, immediate, constant pool index, block target, etc.),
 * not from the physical MachInsn field. This ensures that the dump
 * reflects the logical meaning of the operand rather than the raw
 * bitfield layout used by the instruction format.
 *
 * Each printed field is padded to a fixed width to keep the output
 * visually aligned across all instructions.
 *
 * Example output:
 *     A=R1        B=R2        C=#3
 *     Ax=BB24     Bx=#500
 *
 * Parameters:
 *   name  - Field label ("A", "B", "C", "Ax", "Bx", ...)
 *   insn  - The originating IR instruction, used to determine operand kind
 *   slot  - Operand index in raw_operands[]
 *   value - The physical integer stored in the MachInsn bitfield
 */
static void dump_slot(const char *name, KlrInsn *insn, int slot, int value)
{
    printf("%s=", name);
    dump_raw_operand(insn, slot, value);

    const int FIELD_WIDTH = 12;
    int printed_len = strlen(name) + 1 + 6; // name + '=' + estimated operand width
    int pad = FIELD_WIDTH - printed_len;
    if (pad < 1) pad = 1;

    while (pad--) putchar(' ');
}

void klm_dump_func(KlMachFunc *fn)
{
    printf("====== Linearization @%s ======\n\n", fn->origin->name);

    KlMachBlock *mb;
    list_foreach(mb, link, &fn->bb_list) {
        printf("%%%s:\n", klr_block_name(mb->origin));

        KlMachInsn *mi;
        vector_foreach(mi, &mb->insns) {
            printf("  [%4d]  %-20s ", mi->pc, opcode_name(mi->code));

            KlrInsn *insn = mi->origin;
            /* Print physical fields based on format. */
            switch (mi->format) {
                case FORMAT_Ax:
                    dump_slot("Ax", insn, 0, mi->Ax);
                    break;

                case FORMAT_Axx:
                    dump_slot("Axx", insn, 0, mi->Axx);
                    break;

                case FORMAT_Axxx:
                    dump_slot("Axxx", insn, 0, mi->Axxx);
                    break;

                case FORMAT_ABC:
                    dump_slot("A", insn, 0, mi->A);
                    dump_slot("B", insn, 1, mi->B);
                    dump_slot("C", insn, 2, mi->C);
                    break;

                case FORMAT_AxBx:
                    dump_slot("Ax", insn, 0, mi->Ax);
                    dump_slot("Bx", insn, 1, mi->Bx);
                    break;

                case FORMAT_ABxx:
                    dump_slot("A", insn, 0, mi->A);
                    dump_slot("Bxx", insn, 1, mi->Bxx);
                    break;

                default:
                    printf("<invalid-format>");
                    break;
            }

            /* Print branch targets if present. */
            if (mi->target_true || mi->target_false) {
                printf("  ; targets: ");

                if (mi->target_true) printf("T->%d ", mi->target_true->start_pc);

                if (mi->target_false) printf("F->%d ", mi->target_false->start_pc);
            }

            printf("\n");
        }

        printf("\n");
    }

    printf("==========================================\n");
}

static int klr_do_cgen(KlrFunc *fn, void *data)
{
    log_info("cgen for func '%s'", fn->name);

    KlMachFunc *mfn = klm_linearize_func(fn);
    klm_dump_func(mfn);

    return 0;
}

static KlrPass cgen_pass = {
    .name = "cgen-pass",
    .run = klr_do_cgen,
};

void build_cgen_pm(KlrPassManager *pm, int dump) { pm_add_pass(pm, &cgen_pass, dump); }

#ifdef __cplusplus
}
#endif
