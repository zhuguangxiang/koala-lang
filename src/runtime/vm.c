/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "vm.h"
#include <unistd.h>
#include "atom.h"
#include "log.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* max stack size */
#define MAX_STACK_SIZE (64 * 1024)

/* pthread */
__thread ThreadState *__ts;

KoalaState *kl_new_ks(void)
{
    KoalaState *ks = mm_alloc_obj(ks);
    ks->ts = __ts;
    ks->stack_base = aligned_alloc(16, sizeof(TValue) * MAX_STACK_SIZE);
    ks->stack_top = ks->stack_base;
    ks->stack_size = MAX_STACK_SIZE;
    return ks;
}

void kl_free_ks(KoalaState *ks)
{
    if (!ks) return;
    ASSERT(!ks->cf);
    free(ks->stack_base);
    mm_free(ks);
}

void koala_initialize(void)
{
    /* init logger */
    init_log(LOG_INFO, NULL, 0);

    /* init atom string table */
    init_atom();

    /* init global module table */
    kl_init_mo_stbl();

    /* init builtin & sys module */
    init_builtin_module();
    // init_sys_module(ks);

    /* initialize main thread as koala thread */
    ThreadState *ts = mm_alloc_obj(ts);
    ts->current = kl_new_ks();
    __ts = ts;
}

void koala_run_file(char *path) { /* load klc and run */ }

void koala_finalize(void) { /* finalize atom string table */ fini_atom(); }

#ifdef __cplusplus
}
#endif
