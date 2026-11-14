/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* pthread_getattr_np */
#endif
#include "vm.h"
#include <pthread.h>
#include "atom.h"
#include "excobj.h"
#include "log.h"
#include "mm.h"
#include "modobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/* max stack size */
#define MAX_STACK_SIZE (2 * 64 * 1024)

/* native stack kept free below c_stack_limit for C helpers and the runtime */
#define C_STACK_MARGIN (256 * 1024)

/* pthread */
__thread ThreadState *__ts;

/* The calling thread's native stack floor plus a margin. JIT direct self calls
 * (no C frames between levels) check rsp against it. */
static char *c_stack_limit(void)
{
    pthread_attr_t attr;
    void *addr = NULL;
    size_t size = 0;
    if (pthread_getattr_np(pthread_self(), &attr) != 0) return NULL;
    pthread_attr_getstack(&attr, &addr, &size);
    pthread_attr_destroy(&attr);
    if (!addr || size <= 2 * C_STACK_MARGIN) return NULL;
    return (char *)addr + C_STACK_MARGIN;
}

KoalaState *kl_new_ks(void)
{
    KoalaState *ks = mm_alloc_obj(ks);
    ks->ts = __ts;
    ks->stack_base = aligned_alloc(16, sizeof(TValue) * MAX_STACK_SIZE);
    ks->stack_top = ks->stack_base;
    ks->stack_size = MAX_STACK_SIZE;
    ks->stack_limit = ks->stack_base + MAX_STACK_SIZE;
    ks->c_stack_limit = c_stack_limit();
    return ks;
}

void kl_free_ks(KoalaState *ks)
{
    if (!ks) return;
    ASSERT(!ks->cf);
    free(ks->stack_base);
    mm_free(ks);
}

static void kl_run_main(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;

    if (!m->main) return;

    TValue val = kl_call_code(m->main, NULL, 0);
    if (is_error(&val)) {
        Object *exc = pop_exc();
        ASSERT(exc);
        print_exc_and_free(exc);
    }
}

int koala_run_file(char *path)
{
    Object *m = kl_load_module(path);
    if (m) {
        kl_run_main(m);
        return 0;
    }
    fprintf(stderr, "koala: failed to load module '%s'\n", path);
    return -1;
}

static void load_modules(void)
{
    // It's not necessary to load standard builtin module here.
    // because it will be loaded automatically when needed.
    kl_load_module("std/builtin");
}

void koala_initialize(void)
{
    /* init logger */

#ifdef DEBUG_TEST
    init_log(LOG_WARN, NULL, 0);
#else
    init_log(LOG_TRACE, NULL, 0);
#endif

    /* init atom string table */
    init_atom();

    /* init global module table */
    kl_init_module_stbl();

    /* initialize main thread as koala thread */
    ThreadState *ts = mm_alloc_obj(ts);
    ts->current = kl_new_ks();
    __ts = ts;

    load_modules();
}

void koala_finalize(void) { /* finalize atom string table */ fini_atom(); }

#ifdef __cplusplus
}
#endif
