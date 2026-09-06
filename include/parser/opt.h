/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_OPT_H_
#define _KOALA_OPT_H_

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

int type_allowed_to_prop(TypeSpec *ts);

int klr_const_copy_prop_pass(KlrFunc *fn, void *data);

int klr_normalize_pass(KlrFunc *fn, void *data);

int klr_remove_only_jump_block(KlrFunc *fn, void *data);
int klr_bb_branch_folding(KlrFunc *fn, void *data);
int klr_remove_unused_block(KlrFunc *fn, void *data);
int klr_merge_block_pass(KlrFunc *fn, void *data);

int klr_dce_pass(KlrFunc *fn, void *data);

void kl_optimize(KlrModule *m);

void kl_do_ssa(KlrModule *m);
void kl_exit_ssa(KlrModule *m);

void kl_do_sccp(KlrModule *m);

int klr_phi_coalescing_exit_ssa_pass(KlrFunc *fn);
void kl_ssa_opt(KlrModule *m);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OPT_H_ */
