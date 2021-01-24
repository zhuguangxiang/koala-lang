/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "klvm.h"

#ifdef __cplusplus
extern "C" {
#endif

static KLVMFunction *_new_func(KLVMType *ty, const char *name)
{
    KLVMFunction *fn = mm_alloc(sizeof(*fn));
    INIT_VALUE_HEAD(fn, VALUE_FUNC, ty, name);
    vector_init(&fn->locals, sizeof(void *));
    list_init(&fn->bblist);

    // add parameters
    Vector *params = KLVMProtoTypeParams(ty);
    fn->num_params = vector_size(params);
    KLVMType **item;
    vector_foreach(item, params) KLVMAddLocVar(fn, *item, "");

    return fn;
}

KLVMModule *KLVMCreateModule(const char *name)
{
    KLVMModule *m = mm_alloc(sizeof(*m));
    m->name = name;
    vector_init(&m->vars, sizeof(KLVMVar *));
    vector_init(&m->funcs, sizeof(KLVMFunction *));

    m->fn = (KLVMValue *)_new_func(NULL, "__init__");

    return m;
}

void KLVMDestroyModule(KLVMModule *m)
{
    mm_free(m);
}

void KLVMSetValueName(KLVMValue *val, const char *name)
{
    val->name = name;
}

KLVMValue *KLVMAddVar(KLVMModule *m, KLVMType *ty, const char *name)
{
    KLVMVar *var = mm_alloc(sizeof(*var));
    INIT_VALUE_HEAD(var, VALUE_VAR, ty, name);
    var->flags = KLVM_FLAGS_GLOBAL;
    vector_push_back(&m->vars, &var);
    return (KLVMValue *)var;
}

KLVMValue *KLVMAddLocVar(KLVMFunction *fn, KLVMType *ty, const char *name)
{
    KLVMVar *var = mm_alloc(sizeof(*var));
    INIT_VALUE_HEAD(var, VALUE_VAR, ty, name);
    vector_push_back(&fn->locals, &var);
    return (KLVMValue *)var;
}

KLVMValue *KLVMAddFunc(KLVMModule *m, KLVMType *ty, const char *name)
{
    KLVMFunction *fn = _new_func(ty, name);

    // add to module
    vector_push_back(&m->funcs, &fn);

    return (KLVMValue *)fn;
}

KLVMValue *KLVMGetParam(KLVMValue *fn, int index)
{
    KLVMFunction *_fn = (KLVMFunction *)fn;
    int num_params = _fn->num_params;
    if (index < 0 || index >= num_params) {
        printf("error: index %d out of range(0 ..< %d)\n", index, num_params);
        abort();
    }

    KLVMValue *para = NULL;
    vector_get(&_fn->locals, index, &para);
    return para;
}

KLVMBasicBlock *KLVMAppendBasicBlock(KLVMValue *fn, const char *name)
{
    KLVMBasicBlock *bb = mm_alloc(sizeof(*bb));
    list_node_init(&bb->bbnode);
    bb->fn = fn;
    bb->name = name;
    list_init(&bb->insts);
    KLVMFunction *_fn = (KLVMFunction *)fn;
    list_push_back(&_fn->bblist, &bb->bbnode);
    _fn->num_bbs++;
    return bb;
}

KLVMBuilder *KLVMBasicBlockBuilder(KLVMBasicBlock *bb)
{
    KLVMBuilder *bldr = mm_alloc(sizeof(*bldr));
    bldr->bb = bb;
    bldr->rover = &bb->insts;
    return bldr;
}

KLVMValue *KLVMReference(KLVMValue *val, const char *name)
{
    KLVMRef *ref = mm_alloc(sizeof(*ref));
    INIT_VALUE_HEAD(ref, VALUE_REF, val->type, name);
    ref->val = val;
    return (KLVMValue *)ref;
}

KLVMConst *_new_const(void)
{
    KLVMConst *k = mm_alloc(sizeof(*k));
    k->kind = VALUE_CONST;
    return k;
}

KLVMValue *KLVMConstInt32(int32_t ival)
{
    KLVMConst *k = _new_const();
    k->type = KLVMInt32Type();
    k->i32val = ival;
    return (KLVMValue *)k;
}

#ifdef __cplusplus
}
#endif
