/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "opt.h"
#include "cmd.h"
#include "pass.h"

#ifdef __cplusplus
extern "C" {
#endif

/* const-copy-prop */
static KlrPass const_copy_prop_pass = {
    .name = "const-copy-prop",
    .run = klr_const_copy_prop_pass,
};

/* normalize */
static KlrPass normalize_pass = {
    .name = "normalize",
    .run = klr_normalize_pass,
};

/* cfg-bb-opt */
static KlrPass cfg_remove_only_jump_pass = {
    .name = "cfg-remove-only-jump",
    .run = klr_remove_only_jump_block,
};

static KlrPass cfg_branch_folding_pass = {
    .name = "cfg-branch-folding",
    .run = klr_bb_branch_folding,
};

static KlrPass cfg_remove_unused_pass = {
    .name = "cfg-remove-unused",
    .run = klr_remove_unused_block,
};

static KlrPass cfg_merge_block_pass = {
    .name = "cfg-merge-block",
    .run = klr_merge_block,
};

/* dce */
static KlrPass dce_pass = {
    .name = "dead-code-elimination",
    .run = klr_dce_pass,
};

void kl_optimize(KlrModule *m)
{
    if (!m || m->errors > 0) return;

    int dump = dump_ir_enabled();

    KlrPassManager pm;
    pm_init(&pm, "opt_pass");

    // const_copy_prop_pass
    pm_add_pass(&pm, &const_copy_prop_pass, dump);
    // normalize_pass
    pm_add_pass(&pm, &normalize_pass, dump);

    // cfg_bb_opt_pass
    KlrPassManager cfg_bb_opt_pm;
    pm_init(&cfg_bb_opt_pm, "cfg_bb_opt_pass");

    pm_add_pass(&cfg_bb_opt_pm, &cfg_remove_only_jump_pass, 0);
    pm_add_pass(&cfg_bb_opt_pm, &cfg_branch_folding_pass, 0);
    pm_add_pass(&cfg_bb_opt_pm, &cfg_remove_unused_pass, 0);
    pm_add_pass(&cfg_bb_opt_pm, &cfg_merge_block_pass, 0);

    pm_add_pm_as_pass(&pm, &cfg_bb_opt_pm, dump);

    // dce_pass
    pm_add_pass(&pm, &dce_pass, dump);

    KlrFunc *fn;
    func_foreach(fn, m) {
        if (!fn) continue;
        pm.run(fn, &pm);
    }

    KlrKlass *kls;
    vector_foreach(kls, &m->klasses) {
        ASSERT(kls);
        KlrFunc *fn;
        func_foreach(fn, kls) {
            pm.run(fn, &pm);
        }
    }

    pm_fini(&cfg_bb_opt_pm);
    pm_fini(&pm);
}

static void run_pipeline(KlrPassManager *pm, KlrModule *m)
{
    KlrFunc *fn;
    func_foreach(fn, m) {
        if (!fn) continue;
        pm->run(fn, pm);
    }

    KlrKlass *kls;
    vector_foreach(kls, &m->klasses) {
        ASSERT(kls);
        KlrFunc *fn;
        func_foreach(fn, kls) {
            pm->run(fn, pm);
        }
    }
}

void kl_ssa_opt(KlrModule *m)
{
    if (!m || m->errors > 0) return;

    int dump = dump_ir_enabled();

    KlrPassManager pm;
    pm_init(&pm, "ssa_opt_pass");

    // normalize_pass
    pm_add_pass(&pm, &normalize_pass, dump);

    // 1. cfg_bb_opt_pass
    KlrPassManager cfg_bb_opt_pm;
    pm_init(&cfg_bb_opt_pm, "cfg_bb_ssa_opt_pass");

    pm_add_pass(&cfg_bb_opt_pm, &cfg_remove_only_jump_pass, 0);
    pm_add_pass(&cfg_bb_opt_pm, &cfg_remove_unused_pass, 0);
    pm_add_pass(&cfg_bb_opt_pm, &cfg_merge_block_pass, 0);

    pm_add_pm_as_pass(&pm, &cfg_bb_opt_pm, dump);

    // 2. dce_pass
    pm_add_pass(&pm, &dce_pass, dump);

    run_pipeline(&pm, m);

    kl_do_ssa(m);

    kl_do_sccp(m);

    kl_exit_ssa(m);
    run_pipeline(&pm, m);

    pm_fini(&cfg_bb_opt_pm);
    pm_fini(&pm);
}

#ifdef __cplusplus
}
#endif
