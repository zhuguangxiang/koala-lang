/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "koala.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue _hello(TValue *self, TValue *args, int nargs)
{
    printf("hello, world\n");
    return none_value;
}

void test_cfunc(void)
{
    Object *m = kl_new_module("cfunc");
    Object *obj = kl_new_cfunc("hello", _hello, m);
    kl_mo_add_func(m, obj);

    kl_init_module(m);
    kl_dump_module(m);

    TValue self = obj_value(obj);
    TValue ret = kl_do_call(&self, NULL, 0);
    ASSERT(is_none(&ret));
}

int main(int argc, char *argv[])
{
    koala_initialize();
    test_cfunc();
    koala_finalize();
    return 0;
}

#ifdef __cplusplus
}
#endif
