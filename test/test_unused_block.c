/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
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
void klr_remove_only_jump_block(KlrFunc *func);
void klr_remove_unused_block(KlrFunc *func);

static void build_unreach_block(KlrModule *m)
{
    KlrValue *fn = klr_add_func(m, int64_type_spec(), "add");
    klr_func_add_param(fn, int64_type_spec(), "a");
    klr_func_add_param(fn, int64_type_spec(), "b");

    KlrValue *v1 = klr_func_get_param(fn, 0);
    KlrValue *v2 = klr_func_get_param(fn, 1);

    KlrBasicBlock *entry = klr_append_block(fn, "entry");
    KlrBuilder bldr;
    klr_builder_head(&bldr, entry);

    KlrValue *t1 = klr_build_add(&bldr, v1, v2, "");
    KlrValue *ret = klr_add_local(&bldr, int64_type_spec(), "res");
    klr_build_store(&bldr, ret, t1);
    klr_build_ret(&bldr, ret);

    KlrBasicBlock *bb2 = klr_append_block(fn, "test_bb");
    klr_builder_head(&bldr, bb2);
    KlrValue *t2 = klr_build_sub(&bldr, v1, klr_const_int(20, int64_type_spec()), "");
    klr_build_ret(&bldr, t2);

    klr_build_jmp(&bldr, bb2);

    klr_print_func((KlrFunc *)fn, stdout);

    KLR_PASS_GROUP(grp);
    register_dot_passes(&grp);
    klr_run_pass_group(&grp, (KlrFunc *)fn);
    klr_fini_pass_group(&grp);

    klr_remove_unused_block((KlrFunc *)fn);
    klr_print_func((KlrFunc *)fn, stdout);
}

int main(int argc, char *argv[])
{
    init_atom();
    init_log(LOG_INFO, NULL, 0);
    typespec_init();
    KlrModule *m = klr_create_module("example");
    build_unreach_block(m);
    fini_log();
    fini_atom();
    return 0;
}

#ifdef __cplusplus
}
#endif
