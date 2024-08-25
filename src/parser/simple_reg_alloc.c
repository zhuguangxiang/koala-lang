/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "bitset.h"
#include "ir.h"
#include "mm.h"

/* codegen: simple register allocation */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * allocate registers directly, no any optimization for debug version
 */
void klr_simple_alloc_registers(KlrFunc *func)
{
    int num_regs = 0;

    /* parameters */
    KlrParam **param;
    vector_foreach(param, &func->params) {
        (*param)->vreg = num_regs++;
#ifndef NOLOG
        fprintf(stdout, "name: ");
        klr_print_name_or_tag((KlrValue *)(*param), stdout);
        fprintf(stdout, ", reg: %d\n", (*param)->vreg);
#endif
    }

    /* local variables */
    KlrLocal **local;
    vector_foreach(local, &func->locals) {
        (*local)->vreg = num_regs++;
#ifndef NOLOG
        fprintf(stdout, "name: ");
        klr_print_name_or_tag((KlrValue *)(*local), stdout);
        fprintf(stdout, ", reg: %d\n", (*local)->vreg);
#endif
    }

    /* instructions */
    KlrBasicBlock *bb;
    KlrInsn *insn;
    basic_block_foreach(bb, func) {
        insn_foreach(insn, bb) {
            if (insn_has_value(insn)) {
                insn->vreg = num_regs++;
#ifndef NOLOG
                fprintf(stdout, "name: ");
                klr_print_name_or_tag((KlrValue *)insn, stdout);
                fprintf(stdout, ", reg: %d\n", insn->vreg);
#endif
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
