/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
func foo(a int, b int) int {
    var c = 100
    c = a + b
    return c
}
*/
void build_foo(KlrModule *m)
{
    TypeDesc *param_types[] = {
        desc_int32(),
        desc_int32(),
        NULL,
    };

    KlrValue *func = klr_add_func(m, desc_int32(), param_types, "foo");

    KlrValue *pa = klr_get_param(func, 0);
    klr_set_name(pa, "a");

    KlrValue *pb = klr_get_param(func, 1);
    klr_set_name(pb, "b");

    KlrBasicBlock *bb = klr_append_block(func, "entry");
    KlrBuilder bldr;
    klr_builder_end(&bldr, bb);

    // var c = 100
    KlrValue *cvar = klr_add_local(&bldr, desc_int32(), "c");
    klr_build_store(&bldr, cvar, klr_const_int32(100));

    // c = a + b
    KlrValue *a = klr_build_load(&bldr, pa);
    KlrValue *b = klr_build_load(&bldr, pb);
    KlrValue *add = klr_build_add(&bldr, a, b, "add");
    klr_build_store(&bldr, cvar, add);

    // return c
    KlrValue *ret = klr_build_load(&bldr, cvar);
    klr_build_ret(&bldr, ret);

    klr_print_func((KlrFunc *)func, stdout);
}

int main(int argc, char *argv[])
{
    KlrModule *m = klr_create_module("example");
    build_foo(m);
    return 0;
}

#ifdef __cplusplus
}
#endif
