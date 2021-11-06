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
    /* FIXME */
    mm_free(m);
}

static KLVMConst *__new_const(void)
{
    KLVMConst *val = mm_alloc_obj(val);
    val->kind = KLVM_VALUE_CONST;
    return val;
}

static KLVMValDef *__new_valdef(TypeDesc *ty, char *name)
{
    KLVMValDef *def = mm_alloc_obj(def);
    def->kind = KLVM_VALUE_REG;
    def->type = ty;
    def->label = name ? name : "";
    def->tag = -1;
    return def;
}

static KLVMVar *__new_var(TypeDesc *ty, char *name)
{
    if (!name) {
        panic("function must have name\n");
    }

    KLVMVar *var = mm_alloc_obj(var);
    var->type = ty;
    var->name = name ? name : "";
    return var;
}

static KLVMBasicBlock *__new_block(KLVMFunc *fn, char *label)
{
    KLVMBasicBlock *bb = mm_alloc_obj(bb);
    bb->func = fn;
    bb->label = label;
    bb->tag = -1;
    init_list(&bb->bb_link);
    init_list(&bb->inst_list);
    init_list(&bb->in_edges);
    init_list(&bb->out_edges);
    return bb;
}

static KLVMFunc *__new_func(TypeDesc *ty, char *name)
{
    if (!name) {
        panic("function must have name\n");
    }

    KLVMFunc *fn = mm_alloc_obj(fn);
    fn->name = name;
    fn->type = ty;

    init_list(&fn->bb_list);
    init_list(&fn->edge_list);
    vector_init_ptr(&fn->locals);

    /* initial 'start' and 'end' dummy block */
    fn->sbb = __new_block(fn, "start");
    fn->ebb = __new_block(fn, "end");

    // add parameters
    Vector *pty = proto_params(ty);
    fn->num_params = vector_size(pty);
    TypeDesc **item;
    vector_foreach(item, pty, { klvm_add_local(fn, *item, null); });

    return fn;
}

KLVMVar *klvm_add_var(KLVMModule *m, char *name, TypeDesc *ty)
{
    KLVMVar *var = __new_var(ty, name);
    vector_push_back(&m->variables, &var);
    return var;
}

KLVMFunc *klvm_add_func(KLVMModule *m, char *name, TypeDesc *ty)
{
    KLVMFunc *fn = __new_func(ty, name);
    vector_push_back(&m->functions, &fn);
    return fn;
}

KLVMFunc *klvm_init_func(KLVMModule *m)
{
    KLVMFunc *fn = m->func;
    if (!fn) {
        TypeDesc *vvty = desc_from_proto(null, null);
        fn = klvm_add_func(m, "__init__", vvty);
        m->func = fn;
    }
    return fn;
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

TypeDesc *klvm_get_func_type(KLVMFunc *fn)
{
    return null;
}

KLVMValue *klvm_get_param(KLVMFunc *fn, int index)
{
    int num_params = fn->num_params;
    if (index < 0 || index >= num_params) {
        panic("index %d out of range(0 ..< %d)\n", index, num_params);
    }

    KLVMVar *param = null;
    vector_get(&fn->locals, index, &param);
    return (KLVMValue *)param;
}

KLVMValue *klvm_add_local(KLVMFunc *fn, TypeDesc *ty, char *name)
{
    KLVMValDef *def = __new_valdef(ty, name);
    vector_push_back(&fn->locals, &def);
    if (!name || !name[0]) def->tag = fn->def_tags++;
    return (KLVMValue *)def;
}

KLVMBasicBlock *klvm_append_block(KLVMFunc *fn, char *label)
{
    KLVMBasicBlock *bb = __new_block(fn, label);

    if (list_empty(&fn->bb_list)) {
        /* first block, add an edge <start, bb> */
        klvm_link_age(fn->sbb, bb);
    }

    list_push_back(&fn->bb_list, &bb->bb_link);
    fn->num_bbs++;
    return bb;
}

KLVMBasicBlock *klvm_add_block(KLVMBasicBlock *bb, char *label)
{
    KLVMFunc *fn = bb->func;
    KLVMBasicBlock *_bb = __new_block(fn, label);
    list_add(&bb->bb_link, &_bb->bb_link);
    fn->num_bbs++;
    return _bb;
}

KLVMBasicBlock *klvm_add_block_before(KLVMBasicBlock *bb, char *label)
{
    KLVMFunc *fn = bb->func;
    KLVMBasicBlock *_bb = __new_block(fn, label);
    list_add_before(&bb->bb_link, &_bb->bb_link);
    fn->num_bbs++;
    return _bb;
}

void klvm_delete_block(KLVMBasicBlock *bb)
{
    /* FIXME */
    list_remove(&bb->bb_link);
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
