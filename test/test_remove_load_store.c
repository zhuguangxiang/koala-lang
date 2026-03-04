/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
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
void klr_insn_remap(KlrFunc *func);

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
    TypeSpec *param_types[] = {
        int64_type_spec(),
        int64_type_spec(),
        NULL,
    };

    KlrValue *func = klr_add_func(m, int64_type_spec(), "foo");
    klr_func_add_param(func, int64_type_spec(), "a");
    klr_func_add_param(func, int64_type_spec(), "b");

    KlrValue *pa = klr_func_get_param(func, 0);
    KlrValue *pb = klr_func_get_param(func, 1);

    KlrBasicBlock *bb = klr_append_block(func, "entry");
    KlrBuilder bldr;
    klr_builder_end(&bldr, bb);

    // var c = 100
    KlrValue *cvar = klr_add_local(&bldr, int64_type_spec(), "c");
    klr_build_store(&bldr, cvar, klr_const_int(100, int64_type_spec()));

    // c = 200 + 300;
    KlrValue *a = klr_const_int(200, int64_type_spec());
    KlrValue *b = klr_const_int(300, int64_type_spec());
    KlrValue *add = klr_build_add(&bldr, a, b, "add");
    klr_build_store(&bldr, cvar, add);

    // d = c + 100;
    KlrValue *dvar = klr_add_local(&bldr, int64_type_spec(), "");
    KlrValue *c = klr_build_load(&bldr, cvar, "");
    add = klr_build_add(&bldr, c, klr_const_int(100, int64_type_spec()), "");
    klr_build_store(&bldr, dvar, add);

    // return c
    KlrValue *ret = klr_build_load(&bldr, dvar, "");
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

    klr_insn_remap((KlrFunc *)func);
    klr_print_func((KlrFunc *)func, stdout);
}

int main(int argc, char *argv[])
{
    init_atom();
    init_log(LOG_INFO, NULL, 0);
    typespec_init();
    KlrModule *m = klr_create_module("example");
    build_foo(m);
    fini_log();
    fini_atom();
    return 0;
}

#ifdef __cplusplus
}
#endif
