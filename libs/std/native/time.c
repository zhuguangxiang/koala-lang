/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <stdint.h>
#include <time.h>
#include "object.h"

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
static TValue sys_sleep_nanos(TValue *self, TValue *args, int nargs)
{
    (void)self;
    ASSERT(nargs == 1);

    int64_t ns = kl_arg_int64(0);
    if (ns <= 0) {
        return none_value;
    }

    struct timespec req;
    req.tv_sec = ns / 1000000000LL;
    req.tv_nsec = ns % 1000000000LL;

    // 简单版：不处理被信号中断的剩余时间
    nanosleep(&req, NULL);

    return none_value;
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
