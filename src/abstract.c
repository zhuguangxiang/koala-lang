/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

Value object_call(Value *self, Value *args, int nargs, Object *kwargs)
{
    if (IS_OBJECT(self)) {
        Object *obj = self->obj;
        Object *default_kwargs = get_default_kwargs(obj);
        CallFunc call = OB_TYPE(obj)->call;
        if (call) {
            return call(self, args, nargs, kwargs);
        } else {
            /* raise an error */
            return ErrorValue();
        }
    }

    if (IS_INT(self)) {
        NIY();
    }

    if (IS_FLOAT(self)) {
        NIY();
    }
}

Value call_method(Value *self, Value *args, int nargs, Object *kwargs)
{
    return NoneValue();
}

#ifdef __cplusplus
}
#endif
