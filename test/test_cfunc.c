/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cfuncobject.h"
#include "log.h"
#include "moduleobject.h"
#include "object.h"
#include "run.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value _hello(Value *self, Value *args, int nargs)
{
    ASSERT(IS_OBJECT(self));
    Object *obj = self->obj;
    ASSERT(IS_MODULE(obj));
    ASSERT(args == NULL);
    ASSERT(nargs == 0);
    printf("hello, world\n");
    return NoneValue;
}

MethodDef method = {
    "hello",
    _hello,
};

void test_cfunc(void)
{
    Object *m = kl_new_module("cfunc");
    Object *obj = kl_new_cfunc(&method, m, NULL);
    module_add_code(m, obj);
    Value ret = object_call(obj, NULL, 0);
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
