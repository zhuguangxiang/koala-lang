/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "kl_llvm.h"

#ifdef __cplusplus
extern "C" {
#endif

void kl_llvm_fini_ctx(kl_llvm_ctx_t *ctx)
{
}

static LLVMTypeRef _llvm_type(kl_llvm_ctx_t *ctx, TypeDesc *desc)
{
}

static kl_llvm_func_t *kl_llvm_function(kl_llvm_ctx_t *ctx, CodeObject *co)
{
    kl_llvm_func_t *fn = mm_alloc(sizeof(*fn));
    fn->co = co;
    fn->llfunc = LLVMAddFunction(ctx->llmod, ctx->mo->name, NULL);
    int narg = code_narg(co);

    vector_init(&fn->locals, sizeof(kl_llvm_value_t));
    LLVMTypeRef lltype;
    LLVMValueRef llvalue;
    locvar_t *var;
    vector_foreach(var, &co->locals)
    {
        lltype = _llvm_type(ctx, var->desc);
        if (i < narg)
            llvalue = LLVMGetParam(fn->llfunc, i);
        else
            llvalue = LLVMBuildAlloca(ctx->builder, lltype, var->name);
        kl_llvm_value_t val = { { var->desc, lltype }, llvalue };
        vector_push_back(&fn->locals, &val);
    }
}

void translate(kl_llvm_ctx_t *ctx);

static void _visit(char *name, Object *obj, void *arg)
{
    kl_llvm_ctx_t *ctx = arg;
    printf("translate: %s\n", name);

    if (method_check(obj)) {
        MethodObject *mob = (MethodObject *)obj;
        if (mob->kind == KFUNC_KIND) {
            printf("translate function\n");
            ctx->func = kl_llvm_function(ctx, mob->ptr);
            translate(ctx);
        }
        else {
            abort();
        }
    }
    else if (field_check(obj)) {
        printf("translate variable\n");
    }
    else if (type_check(obj)) {
        printf("translate klass\n");
    }

    abort();
}

void kl_llvm_translate(kl_llvm_ctx_t *ctx, TypeObject *mo)
{
    ctx->mo = mo;
    ctx->builder = LLVMCreateBuilder();
    vector_init(&ctx->stack, sizeof(kl_llvm_value_t));
    HashMap *mtbl = mo->mtbl;
    if (!mtbl) return;
    mtbl_visit(mtbl, _visit, ctx);
}

#ifdef __cplusplus
}
#endif
