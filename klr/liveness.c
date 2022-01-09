/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klvm.h"

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
        if (!bits_test(bb->def_set, (*item)->vreg)) {
            bits_set(bb->use_set, (*item)->vreg);
        }
    });
    vector_fini(&uses);

    KLVMValue *def = klvm_get_def(inst);
    if (def) bits_set(bb->def_set, def->vreg);
}

void klvm_analyze_liveness(KLVMFunc *fn)
{
    klvm_compute_inst_positions(fn);

    KLVMBasicBlock *bb;
    KLVMInst *inst;

    /* initialize bit vectors */
    int vregs = fn->vregs;
    basic_block_foreach(bb, fn, {
        bb->def_set = bits_new(vregs);
        bb->use_set = bits_new(vregs);
        bb->live_in_set = bits_new(vregs);
        bb->live_out_set = bits_new(vregs);
    });

    /* analyze use & def */
    basic_block_foreach(
        bb, fn, { inst_foreach(inst, bb, { __analyze_use_def(bb, inst); }); });
}

#ifdef __cplusplus
}
#endif
