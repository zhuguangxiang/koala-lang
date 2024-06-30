/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "abstract.h"
#include "cfuncobject.h"
#include "log.h"
#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value _hello(Value *self, Value *unused)
{
    ASSERT(IS_OBJECT(self));
    Object *obj = self->obj;
    ASSERT(IS_CFUNC(obj));
    ASSERT(unused == NULL);
    printf("hello, world\n");
    return NoneValue();
}

MethodDef method = {
    "hello",
    _hello,
    METH_NO_ARGS,
};

void test_cfunc(void)
{
    Object *obj = kl_new_cfunc(&method);
    Value obj_val = ObjectValue(obj);
    Value ret = object_call(&obj_val, NULL, 0, NULL);
    ASSERT(IS_NONE(&ret));
}

int main(int argc, char *argv[])
{
    init_log(LOG_INFO, NULL, 0);
    kl_init(argc, argv);
    test_cfunc();
    kl_fini();
    return 0;
}

#ifdef __cplusplus
}
#endif
