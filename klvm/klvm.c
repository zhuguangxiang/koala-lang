/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klvm.h"
#include "util/log.h"

#ifdef __cplusplus
extern "C" {
#endif

KLVMModule *klvm_create_module(char *name)
{
    KLVMModule *m = mm_alloc_obj(m);
    m->name = name;
    vector_init_ptr(&m->variables);
    vector_init_ptr(&m->functions);
    return m;
}

void klvm_destroy_module(KLVMModule *m)
{
    mm_free(m);
}

static KLVMVar *__new_var(TypeDesc *ty, char *name)
{
    if (!name || !name[0]) {
        panic("var must have name\n");
    }

    KLVMVar *var = mm_alloc_obj(var);
    var->kind = KLVM_VALUE_VAR;
    init_list(&var->use_list);
    var->name = name;
    var->type = ty;
    return var;
}

static KLVMBasicBlock *__new_block(KLVMFunc *fn, char *name)
{
    KLVMBasicBlock *bb = mm_alloc_obj(bb);
    bb->kind = KLVM_VALUE_BLOCK;
    init_list(&bb->use_list);
    bb->name = name ? name : "";

    init_list(&bb->link);
    bb->func = fn;

    init_list(&bb->local_list);
    init_list(&bb->inst_list);
    init_list(&bb->in_edges);
    init_list(&bb->out_edges);
    return bb;
}

static void __add_param(KLVMFunc *fn, TypeDesc *ty, char *name)
{
    KLVMArgument *arg = mm_alloc_obj(arg);
    arg->kind = KLVM_VALUE_ARG;
    init_list(&arg->use_list);
    arg->name = name ? name : "";
    arg->type = ty;
    vector_push_back(&fn->args, &arg);
}

static KLVMFunc *__new_func(TypeDesc *ty, char *name)
{
    if (!name || !name[0]) {
        panic("function must have name\n");
    }

    KLVMFunc *fn = mm_alloc_obj(fn);
    fn->kind = KLVM_VALUE_FUNC;
    init_list(&fn->use_list);
    fn->name = name ? name : "";
    fn->type = ty;

    init_list(&fn->bb_list);
    init_list(&fn->edge_list);
    vector_init_ptr(&fn->args);

    /* initial 'start' and 'end' block */
    fn->sbb = __new_block(fn, "start");
    fn->ebb = __new_block(fn, "end");

    /* add parameters */
    Vector *pty = proto_params(ty);
    TypeDesc **item;
    vector_foreach(item, pty, { __add_param(fn, *item, null); });

    return fn;
}

KLVMValue *klvm_add_var(KLVMModule *m, char *name, TypeDesc *ty)
{
    KLVMVar *var = __new_var(ty, name);
    vector_push_back(&m->variables, &var);
    return (KLVMValue *)var;
}

KLVMFunc *klvm_add_func(KLVMModule *m, char *name, TypeDesc *ty)
{
    KLVMFunc *fn = __new_func(ty, name);
    vector_push_back(&m->functions, &fn);
    return fn;
}

KLVMFunc *klvm_init_func(KLVMModule *m)
{
    KLVMFunc *fn = m->init;
    if (!fn) {
        TypeDesc *vvty = desc_from_proto(null, null);
        fn = klvm_add_func(m, "__init__", vvty);
        m->init = fn;
    }
    return fn;
}

static KLVMConst *__new_const(void)
{
    KLVMConst *val = mm_alloc_obj(val);
    val->kind = KLVM_VALUE_CONST;
    init_list(&val->use_list);
    val->name = "";
    return val;
}

void klvm_free_const(KLVMConst *konst)
{
    mm_free(konst);
}

KLVMValue *klvm_const_int8(int8 v)
{
    KLVMConst *val = __new_const();
    val->type = desc_from_int8();
    val->ival = v;
    return (KLVMValue *)val;
}

KLVMValue *klvm_const_int16(int16 v)
{
    KLVMConst *val = __new_const();
    val->type = desc_from_int16();
    val->ival = v;
    return (KLVMValue *)val;
}

KLVMValue *klvm_const_int32(int32 v)
{
    KLVMConst *val = __new_const();
    val->type = desc_from_int32();
    val->ival = v;
    return (KLVMValue *)val;
}

KLVMValue *klvm_const_int64(int64 v)
{
    KLVMConst *val = __new_const();
    val->type = desc_from_int64();
    val->i64val = v;
    return (KLVMValue *)val;
}

KLVMValue *klvm_const_float32(float32 v)
{
    KLVMConst *val = __new_const();
    val->type = desc_from_float32();
    val->fval = v;
    return (KLVMValue *)val;
}

KLVMValue *klvm_const_float64(float64 v)
{
    KLVMConst *val = __new_const();
    val->type = desc_from_float64();
    val->f64val = v;
    return (KLVMValue *)val;
}

KLVMValue *klvm_const_bool(int8 v)
{
    KLVMConst *val = __new_const();
    val->type = desc_from_bool();
    val->bval = v;
    return (KLVMValue *)val;
}

KLVMValue *klvm_const_char(int32 ch)
{
    KLVMConst *val = __new_const();
    val->type = desc_from_char();
    val->cval = ch;
    return (KLVMValue *)val;
}

KLVMValue *klvm_const_str(char *s)
{
    KLVMConst *val = __new_const();
    val->type = desc_from_str();
    val->sval = s;
    return (KLVMValue *)val;
}

TypeDesc *klvm_get_functype(KLVMFunc *fn)
{
    return fn->type;
}

KLVMValue *klvm_get_param(KLVMFunc *fn, int index)
{
    int num_args = vector_size(&fn->args);
    if (index < 0 || index >= num_args) {
        panic("index %d out of range(0 ..< %d)\n", index, num_args);
    }

    KLVMValue *arg = null;
    vector_get(&fn->args, index, &arg);
    return arg;
}

void klvm_set_name(KLVMValue *val, char *name)
{
    val->name = name ? name : "";
}

KLVMBasicBlock *klvm_append_block(KLVMFunc *fn, char *label)
{
    KLVMBasicBlock *bb = __new_block(fn, label);

    if (list_empty(&fn->bb_list)) {
        /* first block, add an edge <start, bb> */
        klvm_link_age(fn->sbb, bb);
    }

    list_push_back(&fn->bb_list, &bb->link);

    return bb;
}

KLVMBasicBlock *klvm_add_block(KLVMBasicBlock *bb, char *label)
{
    KLVMFunc *fn = bb->func;
    KLVMBasicBlock *_bb = __new_block(fn, label);
    list_add(&bb->link, &_bb->link);
    return _bb;
}

KLVMBasicBlock *klvm_add_block_before(KLVMBasicBlock *bb, char *label)
{
    KLVMFunc *fn = bb->func;
    KLVMBasicBlock *_bb = __new_block(fn, label);
    list_add_before(&bb->link, &_bb->link);
    return _bb;
}

void klvm_delete_block(KLVMBasicBlock *bb)
{
    list_remove(&bb->link);
    mm_free(bb);
}

void klvm_link_age(KLVMBasicBlock *src, KLVMBasicBlock *dst)
{
    KLVMEdge *edge = mm_alloc_obj(edge);
    edge->src = src;
    edge->dst = dst;
    init_list(&edge->link);
    init_list(&edge->in_link);
    init_list(&edge->out_link);

    KLVMFunc *fn = src->func;
    list_push_back(&fn->edge_list, &edge->link);
    list_push_back(&src->out_edges, &edge->out_link);
    list_push_back(&dst->in_edges, &edge->in_link);
}

/*
void KLVMComputeInsnPositions(KLVMValueRef _fn)
{
    KLVMFuncRef fn = (KLVMFuncRef)_fn;

    int pos = 0;
    KLVMInsnRef insn;
    KLVMBasicBlockRef bb;
    BasicBlockForEach(bb, &fn->bb_list, {
        bb->start = pos;
        InsnForEach(insn, &bb->insns, { insn->pos = pos++; });
        bb->end = pos;
    });
}
*/

#ifdef __cplusplus
}
#endif
