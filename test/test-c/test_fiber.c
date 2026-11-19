/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/* Raw fiber primitives: context switch, ping-pong, C locals across switches,
 * deep C recursion on a fiber stack, region pool reuse. No scheduler here:
 * entry functions switch back to the caller and are never resumed after
 * their final switch. */

#include "fiber.h"
#include <string.h>
#include "testhelp.h"

static FiberCtx main_ctx;

/* ---------- one-shot switch ---------- */

static FiberCtx one_ctx;
static int one_value;

static void one_fn(void *arg)
{
    one_value = *(int *)arg;
    kl_fiber_switch(&one_ctx, &main_ctx);
    abort(); /* never resumed */
}

static void test_one_shot(void)
{
    Region *r = kl_region_get();
    int arg = 42;
    kl_fiber_init(&one_ctx, r->mid, one_fn, &arg);
    test_cond("fiber ctx sp is 8 mod 16 (post-call alignment)",
              ((uintptr_t)one_ctx.sp & 15) == 8);
    kl_fiber_switch(&main_ctx, &one_ctx);
    test_cond("fiber ran and switched back", one_value == 42);
    kl_region_recycle(r);
}

/* ---------- ping-pong between two fibers ---------- */

static FiberCtx a_ctx, b_ctx;
static int seq[64];
static int nseq;

static void a_fn(void *arg)
{
    for (int i = 0; i < 10; i++) {
        seq[nseq++] = 1;
        kl_fiber_switch(&a_ctx, &b_ctx);
    }
    seq[nseq++] = 11;
    kl_fiber_switch(&a_ctx, &main_ctx);
    abort();
}

static void b_fn(void *arg)
{
    for (int i = 0; i < 10; i++) {
        seq[nseq++] = 2;
        kl_fiber_switch(&b_ctx, &a_ctx);
    }
    seq[nseq++] = 22;
    kl_fiber_switch(&b_ctx, &main_ctx);
    abort();
}

static void test_ping_pong(void)
{
    Region *ra = kl_region_get();
    Region *rb = kl_region_get();
    nseq = 0;
    kl_fiber_init(&a_ctx, ra->mid, a_fn, NULL);
    kl_fiber_init(&b_ctx, rb->mid, b_fn, NULL);
    kl_fiber_switch(&main_ctx, &a_ctx);
    /* a finished its loop; let b finish too */
    kl_fiber_switch(&main_ctx, &b_ctx);
    int ok = nseq == 22;
    for (int i = 0; i < 20 && ok; i++) ok = seq[i] == (i % 2 == 0 ? 1 : 2);
    ok = ok && seq[20] == 11 && seq[21] == 22;
    test_cond("ping-pong alternates and both fibers finish", ok);
    kl_region_recycle(ra);
    kl_region_recycle(rb);
}

/* ---------- C locals survive switches ---------- */

static FiberCtx loc_ctx;
static long loc_result;

static void loc_fn(void *arg)
{
    long a = 11, b = 22, c = 33, d = 44, e = 55, f = 66;
    long acc = 0;
    for (int i = 1; i <= 5; i++) {
        kl_fiber_switch(&loc_ctx, &main_ctx);
        acc += a + b + c + d + e + f + i;
    }
    loc_result = acc;
    kl_fiber_switch(&loc_ctx, &main_ctx);
    abort();
}

static void test_locals(void)
{
    Region *r = kl_region_get();
    kl_fiber_init(&loc_ctx, r->mid, loc_fn, NULL);
    for (int i = 0; i < 6; i++) kl_fiber_switch(&main_ctx, &loc_ctx);
    /* sum over i=1..5 of (231 + i) */
    test_cond("C locals preserved across 5 switches", loc_result == 231 * 5 + 15);
    kl_region_recycle(r);
}

/* ---------- deep C recursion on the fiber stack ---------- */

static FiberCtx deep_ctx;
static long deep_result;

static long recurse(long n)
{
    long pad[32];
    pad[0] = n;
    if (n == 0) return pad[0];
    return recurse(n - 1) + pad[0];
}

static void deep_fn(void *arg)
{
    deep_result = recurse(1500);
    kl_fiber_switch(&deep_ctx, &main_ctx);
    abort();
}

static void test_deep_recursion(void)
{
    Region *r = kl_region_get();
    kl_fiber_init(&deep_ctx, r->mid, deep_fn, NULL);
    kl_fiber_switch(&main_ctx, &deep_ctx);
    test_cond("1500-deep C recursion on a 1M fiber stack", deep_result == 1500 * 1501 / 2);
    kl_region_recycle(r);
}

/* ---------- many fibers ---------- */

#define MANY 64
static FiberCtx many_ctx[MANY];
static int many_values[MANY];

static void many_fn(void *arg)
{
    int i = *(int *)arg;
    many_values[i] = i * i;
    kl_fiber_switch(&many_ctx[i], &main_ctx);
    abort();
}

static void test_many(void)
{
    Region *regions[MANY];
    int args[MANY];
    for (int i = 0; i < MANY; i++) {
        args[i] = i;
        regions[i] = kl_region_get();
        kl_fiber_init(&many_ctx[i], regions[i]->mid, many_fn, &args[i]);
    }
    for (int i = 0; i < MANY; i++) kl_fiber_switch(&main_ctx, &many_ctx[i]);
    int ok = 1;
    for (int i = 0; i < MANY; i++) ok = ok && many_values[i] == i * i;
    test_cond("64 fibers each ran once", ok);
    for (int i = 0; i < MANY; i++) kl_region_recycle(regions[i]);
}

/* ---------- region pool ---------- */

static void test_pool(void)
{
    Region *r1 = kl_region_new();
    test_cond("region layout: guard + C stack + VM stack",
              r1->mid == r1->lo + r1->guard + r1->c_size && r1->hi == r1->mid + r1->vm_size);
    memset(r1->mid, 0xAB, 64);
    kl_region_recycle(r1);
    Region *r2 = kl_region_get();
    test_cond("pool returns the recycled region", r2 == r1);
    int zeroed = 1;
    for (int i = 0; i < 64; i++) zeroed = zeroed && r2->mid[i] == 0;
    test_cond("VM stack pages come back zeroed (nil)", zeroed);
    kl_region_recycle(r2);
    kl_region_pool_fini();
}

int main(void)
{
    test_one_shot();
    test_ping_pong();
    test_locals();
    test_deep_recursion();
    test_many();
    test_pool();
    test_report();
    return __failed_tests ? 1 : 0;
}
