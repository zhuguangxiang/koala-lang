/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_PASSES_H_
#define _KOALA_PASSES_H_

#include "ir.h"

#ifdef __cplusplus
extern "C" {
#endif

void register_dot_passes(KlrPassGroup *grp);
void register_let_lit_prop_pass(KlrPassGroup *grp);
void register_var_lit_bb_prop_pass(KlrPassGroup *grp);
void register_dce_pass(KlrPassGroup *grp);
void register_cfg_bb_opt_pass(KlrPassGroup *grp);
void klr_run_default_passes(KlrFunc *fn);
void klr_module_run_default_passes(KlrModule *m);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PASSES_H_ */
