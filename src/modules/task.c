/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * The koala `std/task` module: spawn, yield_now, current, group and timeout
 * over the N:1 scheduler (runtime/sched.c). Task, Channel and Group methods
 * are registered with their types.
 */

#include "chanobj.h"
#include "excobj.h"
#include "groupobj.h"
#include "taskobj.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/* spawn(fn any, args ...any) Task — varargs arrive as a tuple, or absent */
static TValue task_spawn(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1 || nargs == 2);
    Object *fn = to_obj(&args[0]);
    if (!IS_CODE(fn) && !IS_CFUNC(fn)) {
        raise_exc_str("spawn expects a function");
        return error_value;
    }
    TValue *argv = NULL;
    int n = 0;
    if (nargs == 2) {
        Object *tup = to_obj(&args[1]);
        ASSERT(IS_TUPLE(tup));
        n = (int)TUPLE_SIZE(tup);
        argv = TUPLE_ITEMS(tup);
    }
    return obj_value(kl_new_task(fn, argv, n));
}

/* yield_now() — give up the CPU, stay runnable */
static TValue task_yield_now(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    kl_yield();
    return no_value;
}

/* current() Task? — nil in the main coroutine */
static TValue task_current(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    TaskObject *t = __ks()->task;
    return t ? obj_value((Object *)t) : nil_value;
}

/* group() Group */
static TValue task_group(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return obj_value(kl_new_group());
}

/* sys_group_timeout(ns int) Group — the .kl wrapper takes a Duration */
static TValue task_sys_group_timeout(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int64_t ns = kl_arg_int64(0);
    Object *g = kl_new_group();
    kl_group_set_deadline((GroupObject *)g, ns);
    return obj_value(g);
}

void task_native_lib_init(NativeLib *lib)
{
    kl_reg_type(lib, &task_type);
    kl_reg_type(lib, &channel_type);
    kl_reg_type(lib, &channel_iter_type);
    kl_reg_type(lib, &group_type);
    kl_reg_func(lib, "spawn", task_spawn);
    kl_reg_func(lib, "yield_now", task_yield_now);
    kl_reg_func(lib, "current", task_current);
    kl_reg_func(lib, "group", task_group);
    kl_reg_func(lib, "sys_group_timeout", task_sys_group_timeout);
}

#ifdef __cplusplus
}
#endif
