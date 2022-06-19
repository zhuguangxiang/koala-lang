/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#include "klm.h"

#ifdef __cplusplus
extern "C" {
#endif

void klm_init_pkg(KLMPackage *pkg, char *name)
{
    pkg->name = name;
    vector_init_ptr(&pkg->variables);
    vector_init_ptr(&pkg->functions);
    vector_init_ptr(&pkg->types);
}

void klm_fini_pkg(KLMPackage *pkg)
{
}

KLMFunc *klm_add_func(KLMPackage *pkg, char *name, TypeDesc *ty)
{
    KLMFunc *func = mm_alloc_obj(func);
    func->name = name;
    func->ty = ty;
    init_list(&func->cb_list);
    vector_init_ptr(&func->locals);
    vector_push_back(&pkg->functions, &func);
    return func;
}

KLMCodeBlock *klm_append_block(KLMFunc *func, char *name)
{
    KLMCodeBlock *block = mm_alloc_obj(block);
    block->label = name;
    init_list(&block->cb_node);
    vector_init_ptr(&block->insts);
    list_add(&func->cb_list, &block->cb_node);
    block->func = func;
    return block;
}

#ifdef __cplusplus
}
#endif
