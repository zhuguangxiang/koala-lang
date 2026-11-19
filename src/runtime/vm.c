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
#include "gc.h"
#include "gcmodule.h"
#include "jit/jit.h"
#include "log.h"
#include "mm.h"
#include "modobj.h"
#include "kosched.h"

#ifdef __cplusplus
extern "C" {
#endif

/* max stack size */
#define MAX_STACK_SIZE (2 * 64 * 1024)

/* native stack kept free below c_stack_limit for C helpers and the runtime */
#define C_STACK_MARGIN (256 * 1024)

/* pthread */
__thread ThreadState *__ts;

/* The calling thread's native stack floor plus a margin for error handling. */
static char *c_stack_limit(void)
{
    pthread_attr_t attr;
    void *addr = NULL;
    size_t size = 0;
    if (pthread_getattr_np(pthread_self(), &attr) != 0) return NULL;
    int status = pthread_attr_getstack(&attr, &addr, &size);
    pthread_attr_destroy(&attr);
    if (status != 0 || !addr) return NULL;
    if (size <= C_STACK_MARGIN) return (char *)addr + size;
    return (char *)addr + C_STACK_MARGIN;
}

KoalaState *kl_new_ks(void)
{
    KoalaState *ks = mm_alloc_obj(ks);
    ks->ts = __ts;
    void *stack = aligned_alloc(16, sizeof(TValue) * MAX_STACK_SIZE);
    ASSERT(stack != NULL);
    memset(stack, 0, sizeof(TValue) * MAX_STACK_SIZE);
    ks->stack_base = stack;
    ks->stack_top = ks->stack_base;
    ks->stack_size = MAX_STACK_SIZE;
    ks->stack_limit = ks->stack_base + MAX_STACK_SIZE;
    ks->c_stack_limit = c_stack_limit();
    init_list(&ks->sched_link);
    init_list(&ks->all_link);
    return ks;
}

void kl_free_ks(KoalaState *ks)
{
    if (!ks) return;
    ASSERT(!ks->cf);
    free(ks->stack_base);

    CallFrame *cf = ks->cf_cache;
    while (cf) {
        CallFrame *prev = cf->back;
        mm_free(cf);
        cf = prev;
    }

    mm_free(ks);
}

KoalaState *kl_new_co_ks(Region *r)
{
    KoalaState *ks = mm_alloc_obj(ks);
    ks->ts = __ts;
    ks->sched = __ts->sched;
    ks->region = r;
    /* VM stack grows up from mid to hi; fresh pages are zero = nil */
    ks->stack_base = (TValue *)r->mid;
    ks->stack_top = ks->stack_base;
    ks->stack_limit = (TValue *)r->hi;
    ks->stack_size = (int)(r->vm_size / sizeof(TValue));
    /* C stack grows down from mid; keep a margin free for C helpers */
    size_t margin = kl_co_c_margin();
    ks->c_stack_limit = r->lo + r->guard + margin;
    /* modules are shared with the main ks */
    KoalaState *main = __ts->sched->main;
    ks->builtin = main->builtin;
    ks->sys = main->sys;
    ks->state = KS_RUNNABLE;
    init_list(&ks->sched_link);
    init_list(&ks->all_link);
    return ks;
}

void kl_abandon_ks(KoalaState *ks)
{
    if (!ks) return;
    ASSERT(ks->region);

    /* active frames of a parked coroutine are not in the cache */
    CallFrame *cf = ks->cf;
    while (cf) {
        CallFrame *back = cf->back;
        mm_free(cf);
        cf = back;
    }
    cf = ks->cf_cache;
    while (cf) {
        CallFrame *back = cf->back;
        mm_free(cf);
        cf = back;
    }

    if (ks->co_args) mm_free(ks->co_args);
    kl_region_recycle(ks->region);
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

    /* no-op with NOLOG (debug/release); level by KOALA_LOG_LEVEL */
    init_log("koala", LOG_TRACE, NULL, 0);

    /* init atom string table */
    init_atom();

    /* init garbage collector */
    gc_init();
    gc_debug_init();

    /* init global module table */
    kl_init_module_stbl();

    /* init global mock registry (consulted only in test mode) */
    kl_init_mock_stbl();

    /* initialize main thread as koala thread */
    ThreadState *ts = mm_alloc_obj(ts);
    ts->current = kl_new_ks();
    __ts = ts;

    /* the N:1 scheduler; the main ks is the root coroutine */
    ts->sched = kl_sched_new(ts->current);

    load_modules();
}

void koala_finalize(void)
{
    /* print jit stats before gc_fini closes the std streams */
    kl_jit_fini();

    /* clean static types before gc_fini frees the dynamic ones */
    kl_fini_static_types();

    /* abandon parked coroutines and report unjoined task errors; the
     * scheduler stays valid until after gc_fini (TaskObject._free uses it) */
    kl_sched_abandon(__ts->sched);

    /* finalize garbage collector */
    gc_fini();

    /* entries only hold module pointers, safe after gc_fini */
    kl_fini_module_stbl();

    /* finalize global mock registry */
    kl_fini_mock_stbl();

    kl_free_ks(__ts->current);
    kl_sched_destroy(__ts->sched);
    mm_free(__ts);

    /* finalize atom string table */
    fini_atom();

    /* finalize logger */
    fini_log();
}

#ifdef __cplusplus
}
#endif
