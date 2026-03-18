/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "opt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* const-copy-prop */
static KlrPass const_copy_prop_pass = {
    .name = "const-copy-prop",
    .run = klr_const_copy_prop_pass,
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

static KlrPassManager cfg_bb_opt_pm;

/* dce */
static KlrPass dce_pass = {
    .name = "dead-code-elimination",
    .run = klr_dce_pass,
};

void build_opt_pm(KlrPassManager *pm, int dump)
{
    pm_add_pass(pm, &const_copy_prop_pass, dump);

    pm_init(&cfg_bb_opt_pm, "cfg_bb_opt_pass");

    pm_add_pass(&cfg_bb_opt_pm, &cfg_remove_only_jump_pass, 0);
    pm_add_pass(&cfg_bb_opt_pm, &cfg_branch_folding_pass, 0);
    pm_add_pass(&cfg_bb_opt_pm, &cfg_remove_unused_pass, 0);
    pm_add_pass(&cfg_bb_opt_pm, &cfg_merge_block_pass, 0);

    pm_add_pm_as_pass(pm, &cfg_bb_opt_pm, dump);

    pm_add_pass(pm, &dce_pass, dump);
}

#ifdef __cplusplus
}
#endif
