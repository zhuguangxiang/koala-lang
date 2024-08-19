/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ir.h"
#include "log.h"

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
        log_info("running pass %s", pass->name);
        pass->callback(fn, pass->arg);
        log_info("end of pass %s", pass->name);
    }
}

#ifdef __cplusplus
}
#endif
