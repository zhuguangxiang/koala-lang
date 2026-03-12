/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "log.h"
#include "passes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KlrPass {
    List link;
    const char *name;
    KlrPassFunc callback;
    void *arg;
} KlrPass;

void klr_fini_pass_group(KlrPassGroup *grp)
{
    KlrPass *pass, *next;
    list_foreach_safe(pass, next, link, &grp->passes) {
        list_remove(&pass->link);
        mm_free(pass);
    }
}

void klr_add_pass(KlrPassGroup *grp, char *name, KlrPassFunc fn, void *arg)
{
    KlrPass *pass = mm_alloc_obj_fast(pass);
    init_list(&pass->link);
    pass->name = name;
    pass->callback = fn;
    pass->arg = arg;
    list_push_back(&grp->passes, &pass->link);
}

void klr_run_pass_group(KlrPassGroup *grp, KlrFunc *fn)
{
    KlrPass *pass;
    list_foreach(pass, link, &grp->passes) {
        pass->callback(fn, pass->arg);
    }
}

void klr_run_default_passes(KlrFunc *fn)
{
    KLR_PASS_GROUP(grp);
    register_let_lit_prop_pass(&grp);
    // register_var_lit_bb_prop_pass(&grp);
    register_cfg_bb_opt_pass(&grp);
    register_dce_pass(&grp);
    klr_run_pass_group(&grp, fn);
}

void klr_module_run_default_passes(KlrModule *m)
{
    KlrFunc *fn;
    vector_foreach(fn, &m->functions) {
        klr_run_default_passes(fn);
    }
}

#ifdef __cplusplus
}
#endif
