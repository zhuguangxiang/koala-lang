/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <stdint.h>
#include <time.h>
#include "excobj.h"
#include "object.h"
#include "kosched.h"

#ifdef __cplusplus
extern "C" {
#endif

// func sys_clock_realtime_nanos() int {}
static TValue sys_clock_realtime_nanos(TValue *self, TValue *args, int nargs)
{
    (void)self;
    ASSERT(nargs == 0);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    int64_t nsec = (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
    return int64_value(nsec);
}

// func sys_clock_monotonic_nanos() int {}
static TValue sys_clock_monotonic_nanos(TValue *self, TValue *args, int nargs)
{
    (void)self;
    ASSERT(nargs == 0);

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    int64_t nsec = (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
    return int64_value(nsec);
}

// func sys_sleep_nanos(ns int) {}
// Cooperative: parks the current coroutine on a scheduler timer, so other
// coroutines run meanwhile. With no other coroutine it behaves like a
// blocking sleep. @parks
static TValue sys_sleep_nanos(TValue *self, TValue *args, int nargs)
{
    (void)self;
    ASSERT(nargs == 1);

    int64_t ns = kl_arg_int64(0);
    if (ns <= 0) {
        return no_value;
    }

    WaitRecord w;
    kl_wait_init(&w, WAIT_TIMER);
    w.timer.deadline = kl_now_ns() + ns;
    kl_timer_add(&w.timer);
    int r = kl_park(&w);
    if (r == KL_PARK_CANCELLED) {
        raise_exc_str("Cancelled");
        return error_value;
    }
    /* KL_PARK_TIMEOUT is the normal wake-up of a sleep */
    return no_value;
}

void time_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "sys_clock_realtime_nanos", sys_clock_realtime_nanos);
    kl_reg_func(lib, "sys_clock_monotonic_nanos", sys_clock_monotonic_nanos);
    kl_reg_func(lib, "sys_sleep_nanos", sys_sleep_nanos);
}

#ifdef __cplusplus
}
#endif
