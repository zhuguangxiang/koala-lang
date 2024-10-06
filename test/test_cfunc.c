/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cfuncobject.h"
#include "log.h"
#include "moduleobject.h"
#include "object.h"
#include "run.h"
#include "stringobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static Value _hello(Value *self)
{
    Object *obj = as_obj(self);
    ASSERT(IS_MODULE(obj));
    printf("hello, world\n");
    Object *v = kl_new_str("hello");
    printf("str: %p\n", v);
    printf("again: hello, world\n");
    v = kl_new_str("world");
    StrObject *sobj = (StrObject *)v;
    printf("str: %s\n", (char *)(sobj->array + 1));
    // panic: no more memory for 330B
    // v = kl_new_str("world121");
    return none_value;
}

static MethodDef method = {
    "hello",
    _hello,
    METH_NO_ARGS,
};

void test_cfunc(void)
{
    Object *m = kl_new_module("cfunc");
    Object *obj = kl_new_cfunc(&method, m, NULL);
    module_add_object(m, "hello", obj);
    Value self = obj_value(obj);
    Value arg = obj_value(m);
    Value ret = object_call(&self, &arg, 1, NULL);
    ASSERT(IS_NONE(&ret));
}

int main(int argc, char *argv[])
{
    init_log(LOG_DEBUG, NULL, 0);
    kl_init(argc, argv);
    test_cfunc();
    kl_fini();
    return 0;
}

#ifdef __cplusplus
}
#endif
