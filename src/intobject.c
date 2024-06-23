/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "eval.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

static int _int_str(KoalaState *ks)
{
#if 0
    // 0 no error, -1 error happen
    return 0;

    int n = kl_args_count(ks);
    Value *v = kl_args_get(ks, 0);
    Value *kw = kl_kwargs_get(ks, "file");
    kl_result_save(ks, "OiiV", 1, 2, 3);

    GC_STACK();
    kl_gc_push(obj);

    void *obj = malloc(1);
    if (!obj) {
        // suspend pthread
        stop_the_world();
    }

    kl_gc_pop();
#endif
    return 1;
}

void call_function(KoalaState *ks)
{
#if 0
    Value *self = get_self(ks);
    if (IS_PRIMITIVE(self)) {
        TypeObject *tp = primitives[self->tag];
        Object *func = tp->ftbl[3];
        if (IS_CFUNC(func)) {
            int ret = func->mdef(ks);
            if (ret) {
                // error
            }
        } else {
            // koala function
        }
    } else if (IS_INTF(self)) {
        Object *func = tp->ftbl[3];
        if (IS_CFUNC(func)) {
            int ret = func->mdef[index](ks);
            if (ret) {
                // error
            }
        } else {
            // koala function
        }
    } else {
        TypeObject *tp = OB_TYPE(self->obj);
        Object *func = tp->ftbl[3];
        if (IS_CFUNC(func)) {
            int ret = func->mdef[index](ks);
            if (ret) {
                // error
            }
        } else {
            // koala function
        }
    }
#endif
}

TypeObject type_type = { OBJECT_HEAD_INIT(&type_type), .tp_name = "Int" };
TypeObject int_type = { OBJECT_HEAD_INIT(&type_type), .tp_name = "Int" };

#ifdef __cplusplus
}
#endif
