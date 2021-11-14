/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klvm.h"
#include "opcode.h"
#include "util/log.h"

#ifdef __cplusplus
extern "C" {
#endif

static void __analyze_use_def(KLVMBasicBlock *bb, KLVMInst *inst)
{
    Vector uses;
    vector_init_ptr(&uses);
    klvm_get_uses(inst, &uses);
    KLVMValue **item;
    vector_foreach(item, &uses, {
        /*
         if a value is DEFINED before in current basic block,
         the USE set does not include it.
         OUT(bb) = Union(IN(S)), S = successor
         IN(bb) = USE || (OUT(bb) - DEF)
        */
        if (!bitvector_get(&bb->defs, (*item)->vreg)) {
            bitvector_set(&bb->uses, (*item)->vreg);
        }
    });
    vector_fini(&uses);

    KLVMValue *def = klvm_get_def(inst);
    if (def) bitvector_set(&bb->defs, def->vreg);
}

void klvm_analyze_liveness(KLVMFunc *fn)
{
    klvm_compute_inst_positions(fn);

    KLVMBasicBlock *bb;
    KLVMInst *inst;

    /* initialize bit vectors */
    int vregs = fn->vregs;
    basic_block_foreach(bb, fn, {
        bitvector_init(&bb->defs, vregs);
        bitvector_init(&bb->uses, vregs);
        bitvector_init(&bb->live_ins, vregs);
        bitvector_init(&bb->live_outs, vregs);
    });

    /* analyze use & def */
    basic_block_foreach(
        bb, fn, { inst_foreach(inst, bb, { __analyze_use_def(bb, inst); }); });
}

#ifdef __cplusplus
}
#endif
