/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* MAP_ANONYMOUS, MAP_STACK */
#endif
#include "fiber.h"
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "log.h"
#include "mm.h"

#if defined(__has_include)
#if __has_include(<valgrind/valgrind.h>)
#include <valgrind/valgrind.h>
#define KOALA_HAVE_VALGRIND 1
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* sizes, configurable through the environment (bytes, K/M/G suffix allowed) */
typedef struct _CoConfig {
    size_t c_size;  /* KOALA_CO_CSTACK, default 1M */
    size_t c_margin; /* KOALA_CO_CMARGIN, default 64K */
    size_t vm_size; /* KOALA_CO_VMSTACK, default 1M */
    int guard;      /* KOALA_CO_GUARD, default 1 */
    int pool_max;   /* KOALA_CO_POOL, default 256 */
    size_t page;
} CoConfig;

static CoConfig co_cfg;
static int co_cfg_ready;

/* bytes, or with K/M/G suffix */
static size_t co_parse_size(const char *s, size_t dflt)
{
    if (!s || !*s) return dflt;
    char *end;
    unsigned long long n = strtoull(s, &end, 10);
    int shift = 0;
    switch (*end) {
        case 'K': case 'k': shift = 10; break;
        case 'M': case 'm': shift = 20; break;
        case 'G': case 'g': shift = 30; break;
        default: break;
    }
    if (n == 0) return dflt;
    return (size_t)n << shift;
}

static void co_config(void)
{
    if (co_cfg_ready) return;
    co_cfg.page = (size_t)sysconf(_SC_PAGESIZE);
    co_cfg.c_size = co_parse_size(getenv("KOALA_CO_CSTACK"), 1024 * 1024);
    co_cfg.c_margin = co_parse_size(getenv("KOALA_CO_CMARGIN"), 64 * 1024);
    co_cfg.vm_size = co_parse_size(getenv("KOALA_CO_VMSTACK"), 1024 * 1024);
    const char *guard = getenv("KOALA_CO_GUARD");
    co_cfg.guard = guard ? atoi(guard) != 0 : 1;
    const char *pool = getenv("KOALA_CO_POOL");
    co_cfg.pool_max = pool ? atoi(pool) : 256;
    if (co_cfg.pool_max < 0) co_cfg.pool_max = 0;
    /* keep both stacks page aligned, the VM stack a multiple of TValue (16) */
    co_cfg.c_size = (co_cfg.c_size + co_cfg.page - 1) & ~(co_cfg.page - 1);
    co_cfg.vm_size = (co_cfg.vm_size + co_cfg.page - 1) & ~(co_cfg.page - 1);
    if (co_cfg.c_margin >= co_cfg.c_size) co_cfg.c_margin = co_cfg.c_size / 2;
    co_cfg_ready = 1;
}

Region *kl_region_new(void)
{
    co_config();
    size_t guard = co_cfg.guard ? co_cfg.page : 0;
    size_t total = guard + co_cfg.c_size + co_cfg.vm_size;
    void *p = mmap(NULL, total, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_STACK, -1, 0);
    if (p == MAP_FAILED) {
        log_fatal("fiber: mmap %zu bytes failed", total);
        return NULL;
    }
    Region *r = mm_alloc_obj(r);
    r->lo = p;
    r->hi = (char *)p + total;
    r->mid = (char *)p + guard + co_cfg.c_size;
    r->size = total;
    r->guard = guard;
    r->c_size = co_cfg.c_size;
    r->vm_size = co_cfg.vm_size;
    r->vg_id = -1;
    r->next = NULL;
    if (guard) mprotect(p, guard, PROT_NONE);
#ifdef KOALA_HAVE_VALGRIND
    r->vg_id = VALGRIND_STACK_REGISTER(r->lo + guard, r->hi);
#endif
    return r;
}

static void region_destroy(Region *r)
{
#ifdef KOALA_HAVE_VALGRIND
    if (r->vg_id >= 0) VALGRIND_STACK_DEREGISTER(r->vg_id);
#endif
    munmap(r->lo, r->size);
    mm_free(r);
}

/* pooled regions, physical pages released and zero-filled on reuse */
static Region *co_pool;
static int co_pool_count;

void kl_region_recycle(Region *r)
{
    if (!r) return;
    /* release physical pages of both stacks; they come back zero-filled,
     * and TAG_NIL == 0, so a reused VM stack is all nil */
    madvise(r->lo + r->guard, r->c_size + r->vm_size, MADV_DONTNEED);
    if (co_pool_count < co_cfg.pool_max) {
        r->next = co_pool;
        co_pool = r;
        co_pool_count++;
    } else {
        region_destroy(r);
    }
}

Region *kl_region_get(void)
{
    co_config();
    if (co_pool) {
        Region *r = co_pool;
        co_pool = r->next;
        co_pool_count--;
        r->next = NULL;
        return r;
    }
    return kl_region_new();
}

void kl_region_pool_fini(void)
{
    while (co_pool) {
        Region *r = co_pool;
        co_pool = r->next;
        region_destroy(r);
    }
    co_pool_count = 0;
}

size_t kl_co_c_margin(void)
{
    co_config();
    return co_cfg.c_margin;
}

void kl_fiber_init(FiberCtx *ctx, void *top, void (*fn)(void *), void *arg)
{
    /* frame that kl_fiber_switch "returns" into, see fiber.h and the plan §5.2:
     *   top-16: trampoline (ret), top-24: rbp, top-32: rbx, top-40: r12 = fn,
     *   top-48: r13 = arg, top-56: r14, top-64: r15, top-72: MXCSR + x87 CW */
    char *t = (char *)top;
    ASSERT(((uintptr_t)t & 15) == 0);
    memset(t - 72, 0, 72);
    *(void **)(t - 16) = (void *)kl_fiber_trampoline;
    *(void **)(t - 40) = (void *)fn;
    *(void **)(t - 48) = arg;
    *(uint32_t *)(t - 72) = 0x1F80; /* default MXCSR */
    *(uint16_t *)(t - 68) = 0x037F; /* default x87 control word */
    ctx->sp = t - 72;
}

#ifdef __cplusplus
}
#endif
