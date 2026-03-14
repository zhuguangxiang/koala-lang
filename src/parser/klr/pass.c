/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

void fini_pipeline(KlrPipeline *grp)
{
    KlrPass *pass, *next;
    list_foreach_safe(pass, next, link, &grp->passes) {
        list_remove(&pass->link);
        mm_free(pass);
    }
}

void pipeline_add_pass(KlrPipeline *grp, KlrPass *pass)
{
    init_list(&pass->link);
    list_push_back(&grp->passes, &pass->link);
    ++grp->count;
}

void run_pipeline(KlrPipeline *grp, KlrFunc *fn)
{
    log_info("run pipeline with %d passes on function '%s'", grp->count, fn->name);

#ifndef NOLOG
    klr_print_func(fn, stdout);
#endif

    int changed = 1;
    int iteration = 1;
    while (changed && iteration < 10) {
        log_info("iteration %d:", iteration);
        ++iteration;
        changed = 0;
        KlrPass *pass;
        list_foreach(pass, link, &grp->passes) {
            changed |= pass->callback(fn, pass->arg);
            log_info("==================After Pass '%s'=================", pass->name);
#ifndef NOLOG
            klr_print_func(fn, stdout);
#endif
        }
    }
}

extern KlrPass const_copy_prop_pass;
extern KlrPass cfg_bb_opt_pass;
extern KlrPass dce_pass;

void run_default_pipeline(KlrFunc *fn)
{
    PIPELINE(pipe);
    pipeline_add_pass(&pipe, &const_copy_prop_pass);
    pipeline_add_pass(&pipe, &cfg_bb_opt_pass);
    pipeline_add_pass(&pipe, &dce_pass);
    run_pipeline(&pipe, fn);
}

void klr_run_default_pipeline(KlrModule *m)
{
    KlrFunc *fn;
    vector_foreach(fn, &m->functions) {
        run_default_pipeline(fn);
    }
}

#ifdef __cplusplus
}
#endif
