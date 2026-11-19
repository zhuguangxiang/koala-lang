/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_FIBER_H_
#define _KOALA_FIBER_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Stackful coroutine primitives (N:1, one pthread runs all coroutines).
 *
 * Each coroutine owns one mmap Region holding both stacks:
 *
 *  low address                                                        high address
 *  ┌───────┬──────────────────────────────┬─────────────────────────────────────┐
 *  │ guard │   C stack (grows down) <──── │ ────> VM TValue stack (grows up)     │
 *  │ page  │ c_stack_limit = lo+G+margin  │ stack_base = mid    stack_limit = hi │
 *  └───────┴──────────────────────────────┴─────────────────────────────────────┘
 *
 * A switch saves only callee-saved state (rbx, rbp, r12-r15, rsp, MXCSR, x87 CW)
 * and happens only inside a native C call, which is a GC safepoint.
 */

/* saved stack pointer of a suspended coroutine; first field read by the asm */
typedef struct _FiberCtx {
    void *sp;
} FiberCtx;

typedef struct _Region {
    /* mapping start (guard page when enabled) */
    char *lo;
    /* mapping end, VM stack limit */
    char *hi;
    /* C stack top / VM stack base */
    char *mid;
    /* total mapping bytes */
    size_t size;
    /* guard bytes, 0 or one page */
    size_t guard;
    /* C stack bytes, mid - lo - guard */
    size_t c_size;
    /* VM stack bytes, hi - mid */
    size_t vm_size;
    /* valgrind stack id, -1 when not registered */
    int vg_id;
    /* pool free list */
    struct _Region *next;
} Region;

/* Allocate a fresh region (mmap, guard page, lazy commitment). */
Region *kl_region_new(void);
/* Return a region to the pool (madvise DONTNEED) or munmap it when the pool is full. */
void kl_region_recycle(Region *r);
/* Get a region from the pool, or allocate a fresh one. */
Region *kl_region_get(void);
/* Free every pooled region, at program exit. */
void kl_region_pool_fini(void);

/* The C-stack margin kept free below c_stack_limit (KOALA_CO_CMARGIN). */
size_t kl_co_c_margin(void);

/* Build the initial frame of a fiber so that kl_fiber_switch "returns" into
 * fn(arg) via kl_fiber_trampoline. `top` is the 16-byte-aligned C stack top
 * (region->mid). The entry function must never return. */
void kl_fiber_init(FiberCtx *ctx, void *top, void (*fn)(void *), void *arg);

/* Save the current context into `from` and resume `to`. Only callee-saved
 * registers are preserved; the caller's C frame stays on its own stack.
 * Defined in fiber_x86_64.S. */
void kl_fiber_switch(FiberCtx *from, FiberCtx *to);

/* First entry of a new fiber: r12 = fn, r13 = arg (see kl_fiber_init). */
void kl_fiber_trampoline(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FIBER_H_ */
