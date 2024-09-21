/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

KlrValue *klr_const_int(int64_t val)
{
    KlrConst *lit = mm_alloc_obj_fast(lit);
    INIT_KLR_VALUE(lit, KLR_VALUE_CONST, &int_desc, "");
    lit->which = CONST_INT;
    lit->len = 0;
    lit->ival = val;
    return (KlrValue *)lit;
}

KlrValue *klr_const_float(double val)
{
    KlrConst *lit = mm_alloc_obj_fast(lit);
    INIT_KLR_VALUE(lit, KLR_VALUE_CONST, &float_desc, "");
    lit->which = CONST_FLT;
    lit->len = 0;
    lit->fval = val;
    return (KlrValue *)lit;
}

KlrValue *klr_const_bool(int v)
{
    KlrConst *lit = mm_alloc_obj_fast(lit);
    INIT_KLR_VALUE(lit, KLR_VALUE_CONST, &bool_desc, "");
    lit->which = CONST_BOOL;
    lit->len = 0;
    lit->bval = v;
    return (KlrValue *)lit;
}

KlrValue *klr_const_string(char *s, int len)
{
    KlrConst *lit = mm_alloc_obj_fast(lit);
    INIT_KLR_VALUE(lit, KLR_VALUE_CONST, &str_desc, "");
    lit->which = CONST_STR;
    lit->len = len;
    lit->sval = s;
    return (KlrValue *)lit;
}

static KlrBasicBlock *new_block(KlrFunc *fn, char *name)
{
    KlrBasicBlock *bb = mm_alloc_obj(bb);
    INIT_KLR_VALUE(bb, KLR_VALUE_BLOCK, NULL, name);

    init_list(&bb->link);
    bb->func = fn;

    init_list(&bb->local_list);
    init_list(&bb->insn_list);
    init_list(&bb->in_edges);
    init_list(&bb->out_edges);
    // init_list(&bb->phi_list);

    // vector_init(&bb->phis, PTR_SIZE);

    return bb;
}

KlrBasicBlock *klr_append_block(KlrValue *fn_val, char *label)
{
    KlrFunc *fn = (KlrFunc *)fn_val;
    KlrBasicBlock *bb = new_block(fn, label);

    if (list_empty(&fn->bb_list)) {
        /* first block, add an edge <start, bb> */
        klr_link_edge(fn->sbb, bb);
    }

    list_push_back(&fn->bb_list, &bb->link);

    return bb;
}

KlrBasicBlock *klr_add_block(KlrBasicBlock *bb, char *label)
{
    KlrFunc *fn = bb->func;
    KlrBasicBlock *_bb = new_block(fn, label);
    list_add(&bb->link, &_bb->link);
    return _bb;
}

KlrBasicBlock *klr_add_block_before(KlrBasicBlock *bb, char *label)
{
    KlrFunc *fn = bb->func;
    KlrBasicBlock *_bb = new_block(fn, label);
    list_add_before(&bb->link, &_bb->link);
    return _bb;
}

void klr_delete_block(KlrBasicBlock *bb)
{
    list_remove(&bb->link);
    mm_free(bb);
}

void klr_link_edge(KlrBasicBlock *src, KlrBasicBlock *dst)
{
    KlrEdge *edge = mm_alloc_obj_fast(edge);
    edge->src = src;
    edge->dst = dst;
    init_list(&edge->link);
    init_list(&edge->in_link);
    init_list(&edge->out_link);

    KlrFunc *fn = src->func;
    list_push_back(&fn->edge_list, &edge->link);
    list_push_back(&src->out_edges, &edge->out_link);
    list_push_back(&dst->in_edges, &edge->in_link);
    ++src->num_outedges;
    ++dst->num_inedges;
}

void klr_remove_edge(KlrEdge *edge)
{
    ASSERT(edge);
    --edge->src->num_outedges;
    --edge->dst->num_inedges;
    list_remove(&edge->link);
    list_remove(&edge->in_link);
    list_remove(&edge->out_link);
    mm_free(edge);
}

KlrModule *klr_create_module(char *name)
{
    KlrModule *m = mm_alloc_obj_fast(m);
    m->name = name;
    vector_init_ptr(&m->globals);
    vector_init_ptr(&m->functions);
    m->init = NULL;
    return m;
}

void klr_destroy_module(KlrModule *m) {}

KlrValue *klr_add_func(KlrModule *m, TypeDesc *ret, TypeDesc **params, char *name)
{
    KlrFunc *fn = mm_alloc_obj(fn);
    INIT_KLR_VALUE(fn, KLR_VALUE_FUNC, ret, name);

    init_list(&fn->bb_list);
    init_list(&fn->edge_list);
    vector_init_ptr(&fn->params);
    vector_init_ptr(&fn->locals);

    /* initial 'start' and 'end' block */
    fn->sbb = new_block(fn, "start");
    fn->ebb = new_block(fn, "end");

    /* add params */
    TypeDesc **item = params;
    while (*item) {
        KlrParam *val = mm_alloc_obj(val);
        INIT_KLR_VALUE(val, KLR_VALUE_PARAM, *item, "");
        vector_push_back(&fn->params, &val);
        ++item;
    }

    vector_push_back(&m->functions, &fn);
    fn->module = m;
    return (KlrValue *)fn;
}

KlrValue *klr_get_param(KlrValue *val, int index)
{
    KlrFunc *func = (KlrFunc *)val;

    int size = vector_size(&func->params);
    if (index < 0 || index >= size) {
        panic("index %d out of range(0 ..< %d)", index, size);
    }

    KlrValue **item = vector_get(&func->params, index);
    return *item;
}

static KlrLocal *new_local(TypeDesc *ty, char *name)
{
    KlrLocal *local = mm_alloc_obj_fast(local);
    INIT_KLR_VALUE(local, KLR_VALUE_LOCAL, ty, name);
    init_list(&local->bb_link);
    local->bb = NULL;
    local->counter = 0;
    return local;
}

KlrValue *klr_add_local(KlrBuilder *bldr, TypeDesc *ty, char *name)
{
    KlrLocal *local = new_local(ty, name);
    KlrBasicBlock *bb = bldr->bb;
    list_push_back(&bb->local_list, &local->bb_link);
    local->bb = bb;
    KlrFunc *func = bb->func;
    vector_push_back(&func->locals, &local);
    return (KlrValue *)local;
}

#ifdef __cplusplus
}
#endif
