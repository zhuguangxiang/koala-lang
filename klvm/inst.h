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

#ifdef __cplusplus
extern "C" {
#endif

struct _KLVMBuilder {
    KLVMBasicBlock *bb;
    List *it;
};

struct _KLVMUse {
    /* parent(use_list) */
    KLVMValue *parent;
    /* link in use_list */
    List use_link;
    /* self */
    KLVMInst *inst;
};

#define use_foreach(use, val, closure) \
    list_foreach(use, use_link, &(val)->use_list, closure)

enum _KLVMOperKind {
    KLVM_OPER_NONE,
    KLVM_OPER_CONST,
    KLVM_OPER_VAR,
    KLVM_OPER_FUNC,
    KLVM_OPER_BLOCK,
    KLVM_OPER_REG,
};

struct _KLVMOper {
    KLVMOperKind kind;
    union {
        KLVMConst *konst;
        KLVMUse *use;
    };
};

struct _KLVMInst {
    KLVM_VALUE_HEAD
    /* linear position in function */
    int pos;
    /* opcode */
    KlOpCode opcode;
    /* number of operands */
    int num_ops;
    /* link in bb */
    List bb_link;
    /* ->bb */
    KLVMBasicBlock *bb;
    /* operands */
    KLVMOper operands[0];
};

/* Set builder at head */
static inline void klvm_builder_head(KLVMBuilder *bldr, KLVMBasicBlock *bb)
{
    bldr->bb = bb;
    bldr->it = &bb->inst_list;
}

/* Set builder at and */
static inline void klvm_builder_end(KLVMBuilder *bldr, KLVMBasicBlock *bb)
{
    bldr->bb = bb;
    bldr->it = bb->inst_list.prev;
}

/* Set builder at 'inst' */
static inline void klvm_builder_at(KLVMBuilder *bldr, KLVMInst *inst)
{
    bldr->it = &inst->bb_link;
}

/* Set builder before 'inst' */
static inline void klvm_builder_before(KLVMBuilder *bldr, KLVMInst *inst)
{
    bldr->it = inst->bb_link.prev;
}

/* instruction iteration */
#define inst_foreach(inst, bb, closure) \
    list_foreach(inst, bb_link, &(bb)->inst_list, closure)

#define inst_foreach_safe(inst, nxt, bb, closure) \
    list_foreach_safe(inst, nxt, bb_link, &(bb)->inst_list, closure)

#define inst_empty(bb) list_empty(&(bb)->inst_list)
#define inst_first(bb) list_first(&(bb)->inst_list, KLVMInst, bb_link)
#define inst_last(bb)  list_last(&(bb)->inst_list, KLVMInst, bb_link)

void klvm_get_uses(KLVMInst *inst, Vector *uses);
KLVMValue *klvm_get_def(KLVMInst *inst);

KLVMValue *klvm_build_local(KLVMBuilder *bldr, TypeDesc *ty, char *name);
void klvm_build_copy(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_add(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_sub(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_mul(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_div(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_mod(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_cmple(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_cmplt(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_cmpge(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_cmpgt(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_cmpeq(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_cmpne(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);

KLVMValue *klvm_build_and(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_or(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_not(KLVMBuilder *bldr, KLVMValue *val);
KLVMValue *klvm_build_neg(KLVMBuilder *bldr, KLVMValue *val);

KLVMValue *klvm_build_band(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_bor(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_bxor(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_bnot(KLVMBuilder *bldr, KLVMValue *val);
KLVMValue *klvm_build_shl(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);
KLVMValue *klvm_build_shr(KLVMBuilder *bldr, KLVMValue *lhs, KLVMValue *rhs);

void klvm_build_jmp(KLVMBuilder *bldr, KLVMBasicBlock *dst);
void klvm_build_condjmp(KLVMBuilder *bldr, KLVMValue *cond,
                        KLVMBasicBlock *_true, KLVMBasicBlock *_false);
void klvm_build_ret(KLVMBuilder *bldr, KLVMValue *ret);
void klvm_build_ret_void(KLVMBuilder *bldr);

KLVMValue *klvm_build_call(KLVMBuilder *bldr, KLVMFunc *fn, KLVMValue *args[],
                           int size);
void klvm_build_field_set(KLVMBuilder *bldr, KLVMValue *obj, char *field,
                          KLVMValue *val);
KLVMValue *klvm_build_field_get(KLVMBuilder *bldr, KLVMValue *obj, char *field);

void klvm_build_index_set(KLVMBuilder *bldr, KLVMValue *obj, int index,
                          KLVMValue *val);
KLVMValue *klvm_build_index_get(KLVMBuilder *bldr, KLVMValue *obj, int index);

void klvm_build_map_set(KLVMBuilder *bldr, KLVMValue *obj, KLVMValue *key,
                        KLVMValue *val);
KLVMValue *klvm_build_map_get(KLVMBuilder *bldr, KLVMValue *obj,
                              KLVMValue *key);

#ifdef __cplusplus
}
#endif

#endif /* _KLVM_INST_H_ */
