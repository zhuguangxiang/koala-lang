/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "codeobject.h"
#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value _code_call(Value *self, Value *args, int nargs, Object *kwargs)
{
    ASSERT(IS_OBJECT(self));
    Object *code = self->obj;
    ASSERT(IS_CODE(code));
    Value result = kl_eval_code(code, args, nargs, kwargs);
    return result;
}

TypeObject code_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "code",
    .call = _code_call,
};

#ifdef __cplusplus
}
#endif
