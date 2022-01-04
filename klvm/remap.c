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

void klvm_map_inst(KLVMInst *inst, KLVMBasicBlock *bb)
{
    if (inst->opcode == OP_COPY) {
        return;
    }

    if (inst->opcode == OP_COND_JMP) {
        KLVMOper *oper = inst->operands[0];
        KLVMValue *val = oper->use->parent;
        if (val->kind == KLVM_VALUE_INST) {
            KLVMInst *parent = (KLVMInst *)val;
            switch (parent->opcode) {
                case OP_CMP_LT:
                    /* code */
                    break;
                case OP_CMP_LE:
                    break;
                default:
                    break;
            }
        }
    }
}

void klvm_remap_pass(KLVMFunc *fn, void *arg)
{
    KLVMBasicBlock *bb;
    KLVMInst *inst;

    // combine KLVM opcode into vm's code
    basic_block_foreach(
        bb, fn, { inst_foreach_safe(inst, bb, { klvm_map_inst(inst, bb); }); });
}

#ifdef __cplusplus
}
#endif
