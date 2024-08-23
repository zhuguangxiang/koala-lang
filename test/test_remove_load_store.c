/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "log.h"
#include "passes.h"

#ifdef __cplusplus
extern "C" {
#endif

void klr_remove_load_pass(KlrFunc *func, void *ctx);
void klr_remove_store_pass(KlrFunc *func, void *ctx);
void klr_constant_folding_pass(KlrFunc *func, void *ctx);
void klr_constant_propagation_pass(KlrFunc *func, void *ctx);
void klr_remove_unused_pass(KlrFunc *func, void *ctx);

/*
func foo(a int, b int) int {
    var c = 100
    c = 200 + 300;
    var d = c + 100;
    return d;
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

    // c = 200 + 300;
    KlrValue *a = klr_const_int32(200);
    KlrValue *b = klr_const_int32(300);
    KlrValue *add = klr_build_add(&bldr, a, b, "add");
    klr_build_store(&bldr, cvar, add);

    // d = c + 100;
    KlrValue *dvar = klr_add_local(&bldr, desc_int32(), "");
    KlrValue *c = klr_build_load(&bldr, cvar);
    add = klr_build_add(&bldr, c, klr_const_int32(100), "");
    klr_build_store(&bldr, dvar, add);

    // return c
    KlrValue *ret = klr_build_load(&bldr, dvar);
    klr_build_ret(&bldr, ret);

    klr_print_func((KlrFunc *)func, stdout);

    printf("remove load instructions\n");
    klr_remove_load_pass((KlrFunc *)func, NULL);
    klr_print_func((KlrFunc *)func, stdout);

    printf("remove store instructions\n");
    klr_remove_store_pass((KlrFunc *)func, NULL);
    klr_print_func((KlrFunc *)func, stdout);

    printf("constant folding\n");
    klr_constant_folding_pass((KlrFunc *)func, NULL);
    klr_print_func((KlrFunc *)func, stdout);

    printf("constant propagation\n");
    klr_constant_propagation_pass((KlrFunc *)func, NULL);
    klr_print_func((KlrFunc *)func, stdout);

    printf("constant folding\n");
    klr_constant_folding_pass((KlrFunc *)func, NULL);
    klr_print_func((KlrFunc *)func, stdout);

    printf("constant propagation\n");
    klr_constant_propagation_pass((KlrFunc *)func, NULL);
    klr_print_func((KlrFunc *)func, stdout);

    printf("constant folding\n");
    klr_constant_folding_pass((KlrFunc *)func, NULL);
    klr_print_func((KlrFunc *)func, stdout);

    klr_remove_unused_pass((KlrFunc *)func, NULL);
}

int main(int argc, char *argv[])
{
    init_log(LOG_INFO, NULL, 0);
    KlrModule *m = klr_create_module("example");
    build_foo(m);
    return 0;
}

#ifdef __cplusplus
}
#endif
