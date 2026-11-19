/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "taskobj.h"
#include "excobj.h"
#include "gc.h"
#include "groupobj.h"
#include "mm.h"
#include "shadowstack.h"

#ifdef __cplusplus
extern "C" {
#endif

static Object *task_alloc(TypeObject *tp)
{
    TaskObject *t = gc_alloc(sizeof(TaskObject));
    INIT_OBJECT_HEAD(t, tp, 0);
    t->result = nil_value;
    init_list(&t->joiners);
    init_list(&t->group_link);
    init_list(&t->task_link);
    return (Object *)t;
}

Object *kl_new_task(Object *fn, TValue *args, int nargs)
{
    Object *obj = task_alloc(&task_type);
    kl_gc_protect(&obj);
    TaskObject *t = (TaskObject *)obj;
    t->fn = fn;
    t->state = TASK_PENDING;
    KoalaState *ks = kl_spawn(fn, args, nargs);
    t->ks = ks;
    ks->task = t;
    /* args are shared with ks->co_args, freed once when the task finishes */
    t->args = ks->co_args;
    t->nargs = nargs;
    list_push_back(&kl_sched()->tasks, &t->task_link);
    return obj;
}

void kl_task_wake_joiners(TaskObject *t)
{
    List *link;
    while ((link = list_pop_front(&t->joiners))) {
        WaitRecord *w = list_entry(link, WaitRecord, link);
        w->result = KL_PARK_OK;
        kl_ready(w->ks);
    }
}

/* join() any — parks until done; re-raises the child's error message */
static TValue _task_join(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    TaskObject *t = SELF_AS(task_type);

    if (t->state == TASK_PENDING || t->state == TASK_RUNNING) {
        WaitRecord w;
        kl_wait_init(&w, WAIT_JOIN);
        list_push_back(&t->joiners, &w.link);
        int r = kl_park(&w);
        if (r == KL_PARK_CANCELLED) {
            if (!list_empty(&w.link)) list_remove(&w.link);
            raise_exc_str("Cancelled");
            return error_value;
        }
    }

    t->joined = 1;
    if (t->state == TASK_FAILED) {
        const char *msg = t->exc ? kl_exc_get_msg(t->exc) : "unknown error";
        raise_exc_fmt("task failed: %s", msg);
        return error_value;
    }
    if (t->state == TASK_CANCELLED) return nil_value;
    return t->result;
}

/* cancel() */
static TValue _task_cancel(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    TaskObject *t = SELF_AS(task_type);
    kl_cancel_ks(t->ks);
    return no_value;
}

/* done() bool */
static TValue _task_done(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    TaskObject *t = SELF_AS(task_type);
    return bool_value(t->state >= TASK_DONE);
}

static MethodDef task_methods[] = {
    { "join", _task_join },
    { "cancel", _task_cancel },
    { "done", _task_done },
    { NULL },
};

static void task_mark(Object *obj)
{
    TaskObject *t = (TaskObject *)obj;
    gc_mark(t->fn);
    for (int i = 0; i < t->nargs; i++) gc_mark_value(&t->args[i]);
    gc_mark_value(&t->result);
    gc_mark((Object *)t->group);
    kl_exc_mark(t->exc);
}

static void task_free(Object *obj)
{
    TaskObject *t = (TaskObject *)obj;
    list_remove(&t->task_link);
    if (t->group) list_remove(&t->group_link);
    if (t->ks) t->ks->task = NULL;
    /* a detached failed task nobody joined: report it when freed (§6.10) */
    if (t->state == TASK_FAILED && !t->joined && t->exc) {
        print_exc_and_free(t->exc);
        t->exc = NULL;
    }
}

/* pub class Task { ... } */
TypeObject task_type = {
    ._type = &type_type,
    .name = "Task",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(TaskObject),
    ._alloc = task_alloc,
    ._mark = task_mark,
    ._free = task_free,
    .methdefs = task_methods,
};

#ifdef __cplusplus
}
#endif
