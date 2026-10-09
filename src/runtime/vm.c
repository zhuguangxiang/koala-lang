/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "vm.h"
#include "atom.h"
#include "log.h"
#include "mm.h"
#include "modobj.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_tag_mappings(void);

/* max stack size */
#define MAX_STACK_SIZE (2 * 64 * 1024)

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

int koala_test_file(char *path)
{
    Object *m = kl_load_module(path);
    if (m) return kl_run_tests(m);
    fprintf(stderr, "koala: failed to load module '%s'\n", path);
    return -1;
}

static void load_modules(void)
{
    // It's not necessary to load standard builtin module here.
    // because it will be loaded automatically when needed.
    // kl_load_module("std/builtin");
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

    /* initialize tag mappings */
    init_tag_mappings();

    load_modules();
}

void koala_finalize(void) { /* finalize atom string table */ fini_atom(); }

#ifdef __cplusplus
}
#endif
