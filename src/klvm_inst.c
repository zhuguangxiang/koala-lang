/*----------------------------------------------------------------------------*\
|* This file is part of the KLVM project, under the MIT License.              *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void _inst_append(klvm_builder_t *bldr, klvm_inst_t *inst)
{
    klvm_block_t *bb = bldr->bb;
    klvm_inst_t *rover = (klvm_inst_t *)bldr->rover;
    if (!rover)
        list_push_back(&bb->insts, &inst->node);
    else
        list_add(&rover->node, &inst->node);
    bldr->rover = (klvm_value_t *)inst;
    bb->num_insts++;
}

#if 0
int bb_has_terminator(KLVMBasicBlock *bb)
{
    static uint8_t terminators[] = {
        KLVM_INST_RET,
        KLVM_INST_RET_VOID,
        KLVM_INST_BRANCH,
        KLVM_INST_GOTO,
    };
    ListNode *nd = list_last(&bb->insts);
    if (!nd) return 0;

    KLVMInst *last = container_of(nd, KLVMInst, node);
    uint8_t op = last->op;
    for (int i = 0; i < COUNT_OF(terminators); i++) {
        if (op == terminators[i]) {
            printf("error: invalid basic block\n");
            abort();
            return 1;
        }
    }
    return 0;
}
#endif

void klvm_build_copy(klvm_builder_t *bldr, klvm_value_t *rhs, klvm_value_t *lhs)
{
    if (klvm_type_check(lhs->type, rhs->type)) {
        printf("error: type not matched\n");
    }

    klvm_copy_t *inst = malloc(sizeof(*inst));
    KLVM_INIT_INST_HEAD(inst, lhs->type, KLVM_INST_COPY, NULL);
    inst->lhs = lhs;
    inst->rhs = rhs;

    _inst_append(bldr, (klvm_inst_t *)inst);
}

static int _is_cmp(klvm_inst_kind_t op)
{
    static klvm_inst_kind_t cmps[] = {
        KLVM_INST_CMP_EQ,
        KLVM_INST_CMP_NE,
        KLVM_INST_CMP_GT,
        KLVM_INST_CMP_GE,
        KLVM_INST_CMP_LT,
        KLVM_INST_CMP_LE,
    };

    for (int i = 0; i < COUNT_OF(cmps); i++) {
        if (op == cmps[i]) return 1;
    }

    return 0;
}

klvm_value_t *klvm_build_binary(klvm_builder_t *bldr, klvm_inst_kind_t op,
    klvm_value_t *lhs, klvm_value_t *rhs, char *name)
{
    klvm_binary_t *inst = malloc(sizeof(*inst));
    klvm_type_t *ret = lhs->type;
    if (_is_cmp(op)) ret = &klvm_type_bool;
    KLVM_INIT_INST_HEAD(inst, ret, op, name);
    inst->lhs = lhs;
    inst->rhs = rhs;
    _inst_append(bldr, (klvm_inst_t *)inst);
    return (klvm_value_t *)inst;
}

klvm_value_t *klvm_build_call(
    klvm_builder_t *bldr, klvm_value_t *fn, klvm_value_t **args, char *name)
{
    klvm_call_t *inst = malloc(sizeof(*inst));
    klvm_type_t *rty = klvm_proto_return(fn->type);
    KLVM_INIT_INST_HEAD(inst, rty, KLVM_INST_CALL, name);
    inst->fn = fn;
    vector_init(&inst->args, sizeof(void *));

    klvm_value_t **arg = args;
    while (*arg) {
        vector_push_back(&inst->args, arg);
        arg++;
    }

    _inst_append(bldr, (klvm_inst_t *)inst);
    return (klvm_value_t *)inst;
}

void klvm_build_goto(klvm_builder_t *bldr, klvm_block_t *dst)
{
    klvm_goto_t *inst = malloc(sizeof(*inst));
    KLVM_INIT_INST_HEAD(inst, NULL, KLVM_INST_GOTO, NULL);
    inst->dst = dst;
    _inst_append(bldr, (klvm_inst_t *)inst);

    // add edge
    klvm_link_edge(bldr->bb, dst);
}

void klvm_build_branch(klvm_builder_t *bldr, klvm_value_t *cond,
    klvm_block_t *_then, klvm_block_t *_else)
{
    klvm_branch_t *inst = malloc(sizeof(*inst));
    KLVM_INIT_INST_HEAD(inst, NULL, KLVM_INST_BRANCH, NULL);
    inst->cond = cond;
    inst->_then = _then;
    inst->_else = _else;
    _inst_append(bldr, (klvm_inst_t *)inst);

    // add edges
    klvm_link_edge(bldr->bb, _then);
    klvm_link_edge(bldr->bb, _else);
}

void klvm_build_ret(klvm_builder_t *bldr, klvm_value_t *v)
{
    klvm_ret_t *inst = malloc(sizeof(*inst));
    KLVM_INIT_INST_HEAD(inst, v->type, KLVM_INST_RET, NULL);
    inst->ret = v;
    _inst_append(bldr, (klvm_inst_t *)inst);
}

void klvm_build_ret_void(klvm_builder_t *bldr)
{
    klvm_inst_t *inst = malloc(sizeof(*inst));
    KLVM_INIT_INST_HEAD(inst, NULL, KLVM_INST_RET_VOID, NULL);
    _inst_append(bldr, inst);
}

#ifdef __cplusplus
}
#endif
