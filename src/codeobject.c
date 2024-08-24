/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "codeobject.h"
#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject code_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "code",
    .call = kl_eval_code,
};

Object *kl_new_code(char *name, Object *m, TypeObject *cls)
{
    CodeObject *code = mm_alloc_obj(code);
    INIT_OBJECT_HEAD(code, &code_type);
    code->name = name;
    code->module = m;
    code->cls = cls;
    return (Object *)code;
}

#ifdef __cplusplus
}
#endif
