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

void register_constant_folding_pass(KlrPassGroup *grp);
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

    KLR_PASS_GROUP(grp);
    register_constant_folding_pass(&grp);
    klr_run_pass_group(&grp, (KlrFunc *)func);
    klr_fini_pass_group(&grp);

    klr_print_func((KlrFunc *)func, stdout);

    klr_insn_remap((KlrFunc *)func);
    klr_print_func((KlrFunc *)func, stdout);
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
