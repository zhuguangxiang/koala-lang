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
    klr_print_cfg((KlrFunc *)func, stdout);

    KLR_PASS_GROUP(grp);
    register_dot_passes(&grp);
    klr_run_pass_group(&grp, (KlrFunc *)func);
    klr_fini_pass_group(&grp);
}

/*
func fib(n Int32) Int32 {
    if n < 2 { return n; }
    return fib(n - 1) + fib(n - 2)
}
*/

#if 0
void build_fib(KlrModule *m)
{
    TypeDesc *param_types[] = {
        desc_int32(),
        NULL,
    };

    KlrValue *func = klr_add_func(m, desc_int32(), param_types, "fib");
    KlrValue *param = klr_get_param(func, 0);
    klr_set_name(param, "n");

    KlrBasicBlock *entry = klr_append_block(func, "entry");

    KlrBasicBlock *_then = klr_append_block(func, "_then");
    KlrBasicBlock *_else = klr_append_block(func, "_else");

    KlrBuilder bldr;
    klr_builder_end(&bldr, entry);

    /* entry basic block */
    KlrValue *val = klr_build_load(&bldr, param);
    KlrValue *cond = klr_build_cmp(val, klr_const_int32(2));
    klr_build_jmp_lt(&bldr, cond, _then, _else);

    /* _then basic block */
    klr_builder_end(&bldr, _then);
    val = klr_build_load(&bldr, param);
    klr_build_ret(&bldr, val);

    KlrBasicBlock *bb = klr_append_block(func, "");

    /* _else basic block */
    klr_builder_end(&bldr, _else);
    klr_build_jmp(&bldr, bb);

    /* out of if-block */
    klr_builder_end(&bldr, bb);

    val = klr_build_load(&bldr, param);
    KlrValue *sub = klr_build_sub(&bldr, val, klr_const_int32(2));
    KlrValue *args1[] = { sub, NULL };
    KlrValue *ret1 = klr_build_call(&bldr, func, args1);

    val = klr_build_load(&bldr, param);
    sub = klr_build_sub(&bldr, val, klr_const_int32(2));
    KlrValue *args2[] = { sub, NULL };
    KlrValue *ret2 = klr_build_call(&bldr, func, args2);

    KlrValue *ret = klr_build_add(&bldr, ret1, ret2, "");
    klr_build_ret(&bldr, ret);
}
#endif

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
