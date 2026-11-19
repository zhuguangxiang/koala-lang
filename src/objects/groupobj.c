/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "groupobj.h"
#include <string.h>
#include "excobj.h"
#include "gc.h"
#include "mm.h"
#include "shadowstack.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static void group_timer_fire(void *arg)
{
    GroupObject *g = arg;
    g->timed_out = 1;
    g->cancel_requested = 1;
    kl_group_cancel_children(g);
    /* wake the parent parked in __exit__; it re-parks until children die */
    List *link = list_pop_front(&g->waitq);
    if (link) {
        WaitRecord *w = list_entry(link, WaitRecord, link);
        w->result = KL_PARK_OK;
        kl_ready(w->ks);
    }
}

static Object *group_alloc(TypeObject *tp)
{
    GroupObject *g = gc_alloc(sizeof(GroupObject));
    INIT_OBJECT_HEAD(g, tp, 0);
    init_list(&g->children);
    init_list(&g->waitq);
    g->timer.idx = -1;
    g->timer.fire = group_timer_fire;
    g->timer.arg = g;
    g->timer.root = (Object *)g;
    return (Object *)g;
}

Object *kl_new_group(void) { return group_alloc(&group_type); }

void kl_group_set_deadline(GroupObject *g, int64_t ns)
{
    ASSERT(g->timer.idx < 0);
    g->timer.deadline = kl_now_ns() + (ns > 0 ? ns : 0);
    kl_timer_add(&g->timer);
}

Object *kl_group_spawn(GroupObject *g, Object *fn, TValue *args, int nargs)
{
    Object *obj = kl_new_task(fn, args, nargs);
    kl_gc_protect(&obj);
    TaskObject *t = (TaskObject *)obj;
    t->group = g;
    list_push_back(&g->children, &t->group_link);
    g->nlive++;
    return obj;
}

void kl_group_cancel_children(GroupObject *g)
{
    TaskObject *t, *nxt;
    list_foreach_safe(t, nxt, group_link, &g->children) {
        kl_cancel_ks(t->ks);
    }
}

void kl_group_child_done(GroupObject *g, TaskObject *t)
{
    list_remove(&t->group_link);
    t->group = NULL;
    g->nlive--;

    if (t->state == TASK_FAILED && !g->failed && !g->timed_out && !g->cancel_requested) {
        g->failed = 1;
        const char *msg = t->exc ? kl_exc_get_msg(t->exc) : "unknown error";
        g->first_err = strdup(msg);
        /* the group consumed the error */
        t->joined = 1;
        kl_group_cancel_children(g);
    } else if (t->state == TASK_FAILED && (g->failed || g->timed_out || g->cancel_requested)) {
        /* cancelled sibling: the group consumed its Cancelled error too */
        t->joined = 1;
    }

    List *link = list_pop_front(&g->waitq);
    if (link) {
        WaitRecord *w = list_entry(link, WaitRecord, link);
        w->result = KL_PARK_OK;
        kl_ready(w->ks);
    }
}

/* spawn(fn any, args ...any) Task */
static TValue _group_spawn(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1 || nargs == 2);
    GroupObject *g = SELF_AS(group_type);
    Object *fn = to_obj(&args[0]);
    TValue *argv = NULL;
    int n = 0;
    if (nargs == 2) {
        Object *tup = to_obj(&args[1]);
        ASSERT(IS_TUPLE(tup));
        n = (int)TUPLE_SIZE(tup);
        argv = TUPLE_ITEMS(tup);
    }
    return obj_value(kl_group_spawn(g, fn, argv, n));
}

/* __enter__() — the with statement binds the group itself */
static TValue _group_enter(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return no_value;
}

/* __exit__() — join all children; on a child error cancel the rest, then
 * re-raise the first. @parks */
static TValue _group_exit(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    GroupObject *g = SELF_AS(group_type);
    int parent_cancelled = 0;

    while (g->nlive > 0) {
        WaitRecord w;
        kl_wait_init(&w, WAIT_GROUP);
        list_push_back(&g->waitq, &w.link);
        int r = kl_park(&w);
        if (r == KL_PARK_CANCELLED) {
            if (!list_empty(&w.link)) list_remove(&w.link);
            /* a group never leaks running children: cancel and keep waiting */
            parent_cancelled = 1;
            g->cancel_requested = 1;
            kl_group_cancel_children(g);
        }
    }

    kl_timer_remove(&g->timer);
    if (g->failed) {
        raise_exc_fmt("group task failed: %s", g->first_err);
        return error_value;
    }
    if (g->timed_out) {
        raise_exc_str("Timeout");
        return error_value;
    }
    if (parent_cancelled) {
        raise_exc_str("Cancelled");
        return error_value;
    }
    return no_value;
}

static MethodDef group_methods[] = {
    { "spawn", _group_spawn },
    { "__enter__", _group_enter },
    { "__exit__", _group_exit },
    { NULL },
};

static void group_mark(Object *obj)
{
    GroupObject *g = (GroupObject *)obj;
    TaskObject *t;
    list_foreach(t, group_link, &g->children) gc_mark(t);
}

static void group_free(Object *obj)
{
    GroupObject *g = (GroupObject *)obj;
    kl_timer_remove(&g->timer);
    TaskObject *t, *nxt;
    list_foreach_safe(t, nxt, group_link, &g->children) {
        list_remove(&t->group_link);
        t->group = NULL;
    }
    free(g->first_err);
}

/* pub class Group : Withable { ... } */
TypeObject group_type = {
    ._type = &type_type,
    .name = "Group",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(GroupObject),
    ._alloc = group_alloc,
    ._mark = group_mark,
    ._free = group_free,
    .methdefs = group_methods,
};

#ifdef __cplusplus
}
#endif
