/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cgen.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

static void fill_mach_insn(KlMachInsn *mi, KlrInsn *insn)
{
    mi->code = insn->code;
    mi->format = opcode_format(insn->code);
    mi->rd = insn->rd;
    mi->rs = insn->rs;
    mi->rt = insn->rt;
    mi->imm = insn->imm;
    // mi->target_true = insn->bb_true;
    // mi->target_false = insn->bb_false;
    mi->origin = insn;
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

    KlMachBlock *mb;
    list_foreach(mb, link, &mfn->bb_list) {
        KlrBasicBlock *bb = mb->origin;

        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (klr_is_local((KlrValue *)insn)) continue;
            KlMachInsn *mi = mm_alloc_obj(mi);
            fill_mach_insn(mi, insn);
            vector_push_back(&mb->insns, &mi);
        }
    }

    return mfn;
}

void klm_dump_func(KlMachFunc *fn)
{
    printf("====== Linearization @%s ======\n\n", fn->origin->name);

    KlMachBlock *mb;
    list_foreach(mb, link, &fn->bb_list) {
        printf("%%%s:\n", klr_block_name(mb->origin));

        KlMachInsn *mi;
        vector_foreach(mi, &mb->insns) {
            printf("  [%04d]  %-10s", mi->pc, opcode_name(mi->code));

            switch (mi->format) {
                case FORMAT_ABC:
                    printf(" rd=%d rs=%d rt=%d", mi->rd, mi->rs, mi->rt);
                    break;

                case FORMAT_Ax:
                    printf(" rd=%d imm=%d", mi->rd, mi->imm);
                    break;

                case FORMAT_AxBx:
                    printf(" rd=%d rs=%d imm=%d", mi->rd, mi->rs, mi->imm);
                    break;

                    // case FORMAT_JMP:
                    //     printf(" -> %%%s", mi->target_true ? mi->target_true->name :
                    //     "NULL"); break;

                    // case FORMAT_BRANCH2:
                    //     printf(" rs=%d  true=%%%s  false=%%%s", mi->rs,
                    //            mi->target_true ? mi->target_true->name : "NULL",
                    //            mi->target_false ? mi->target_false->name : "NULL");
                    //     break;

                default:
                    break;
            }

            if (mi->target_true) {
                printf(" true=%%%s", klr_block_name(mi->target_true->origin));
            }

            if (mi->target_false) {
                printf(" false=%%%s", klr_block_name(mi->target_false->origin));
            }

            printf("\n");
        }

        printf("\n");
    }

    printf("==========================================\n");
}

#ifdef __cplusplus
}
#endif
