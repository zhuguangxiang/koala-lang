/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KLVM_INST_H_
#define _KLVM_INST_H_

#if !defined(_KLVM_H_)
#error "Only <klvm/klvm.h> can be included directly."
#endif

/* no error here */
#include "klvm.h"
#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _KLVMBuilder {
    KLVMBasicBlock *bb;
    List *it;
};

struct _KLVMUse {
    // link in KLVMInterval
    List use_link;
    // live interval
    KLVMInterval *interval;
    // -> self KLVMInst
    KLVMInst *inst;
};

#define use_foreach(use, use_list, closure) \
    list_foreach(use, use_link, use_list, closure)

enum _KLVMOperKind {
    KLVM_OPER_NONE,
    KLVM_OPER_CONST,
    KLVM_OPER_REG,
    KLVM_OPER_VAR,
    KLVM_OPER_FUNC,
    KLVM_OPER_BLOCK,
    KLVM_OPER_MAX,
};

struct _KLVMOper {
    KLVMOperKind kind;
    union {
        KLVMConst *konst;
        KLVMUse *reg;
        KLVMVar *var;
        KLVMFunc *func;
        KLVMBasicBlock *block;
    };
};

struct _KLVMInst {
    /* linked in KLVMBasicBlock */
    List link;
    /* opcode */
    KLVMOpCode opcode;
    /* number of operands */
    int num_ops;
    /* operands */
    KLVMOper operands[0];
};

/* Set builder at head */
static inline void klvm_set_builder_head(KLVMBuilder *bldr, KLVMBasicBlock *bb)
{
    bldr->bb = bb;
    bldr->it = &bb->inst_list;
}

/* Set builder at tail */
static inline void klvm_set_builder_tail(KLVMBuilder *bldr, KLVMBasicBlock *bb)
{
    bldr->bb = bb;
    bldr->it = bb->inst_list.prev;
}

/* Set builder at 'inst' */
static inline void klvm_set_builder(KLVMBuilder *bldr, KLVMInst *inst)
{
    bldr->it = &inst->link;
}

/* Set builder before 'inst' */
static inline void klvm_set_builder_before(KLVMBuilder *bldr, KLVMInst *inst)
{
    bldr->it = inst->link.prev;
}

/* instruction iteration */
#define inst_foreach(inst, inst_list, closure) \
    list_foreach(inst, link, inst_list, closure)

void klvm_build_copy(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_binary(KLVMBuilder *bldr, KLVMOpCode op, KLVMValue *lhs,
                             KLVMValue *rhs, char *name);
#define klvm_build_add(bldr, lhs, rhs, name) \
    klvm_build_binary(bldr, KLVM_OP_ADD, lhs, rhs, name)
#define klvm_build_sub(bldr, lhs, rhs, name) \
    klvm_build_binary(bldr, KLVM_OP_SUB, lhs, rhs, name)
#define klvm_build_cmple(bldr, lhs, rhs, name) \
    klvm_build_binary(bldr, KLVM_OP_CMP_LE, lhs, rhs, name)
KLVMValue *klvm_build_call(KLVMBuilder *bldr, KLVMFunc *fn, KLVMValue *args[],
                           int size, char *name);
void klvm_build_jmp(KLVMBuilder *bldr, KLVMBasicBlock *dst);
void klvm_build_condjmp(KLVMBuilder *bldr, KLVMValue *cond,
                        KLVMBasicBlock *_then, KLVMBasicBlock *_else);
void klvm_build_ret(KLVMBuilder *bldr, KLVMValue *ret);
void klvm_build_retvoid(KLVMBuilder *bldr);

#ifdef __cplusplus
}
#endif

#endif /* _KLVM_INST_H_ */
