/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "eval.h"
#include "moduleobject.h"

#ifdef __cplusplus
extern "C" {
#endif

extern TypeObject int_type;
extern TypeObject str_type;
extern TypeObject tuple_type;

static void init_types(Object *m)
{
    type_ready(&base_type, m);
    type_ready(&type_type, m);
    type_ready(&int_type, m);
    type_ready(&str_type, m);
    type_ready(&tuple_type, m);
}

void init_builtin_module(KoalaState *ks)
{
    Object *m = kl_new_module("builtin");
    init_types(m);
}

void init_sys_module(KoalaState *ks) {}

#ifdef __cplusplus
}
#endif
