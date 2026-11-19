/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "kosched.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "excobj.h"
#include "gc.h"
#include "groupobj.h"
#include "log.h"
#include "mm.h"
#include "taskobj.h"

#if defined(__SANITIZE_ADDRESS__)
#include <sanitizer/asan_interface.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

int64_t kl_now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
}

/*---------------------------------------------------------------------------+
 |   Timer min-heap                                                          |
 +---------------------------------------------------------------------------*/

static void timer_set(Scheduler *s, int i, TimerNode *t)
{
    s->timers[i] = t;
    t->idx = i;
}

static void timer_up(Scheduler *s, int i)
{
    TimerNode *t = s->timers[i];
    while (i > 0) {
        int p = (i - 1) / 2;
        if (s->timers[p]->deadline <= t->deadline) break;
        timer_set(s, i, s->timers[p]);
        i = p;
    }
    timer_set(s, i, t);
}

static void timer_down(Scheduler *s, int i)
{
    TimerNode *t = s->timers[i];
    for (;;) {
        int l = 2 * i + 1, r = l + 1, m = i;
        if (l < s->ntimers && s->timers[l]->deadline < s->timers[m]->deadline) m = l;
        if (r < s->ntimers && s->timers[r]->deadline < s->timers[m]->deadline) m = r;
        if (m == i) break;
        timer_set(s, i, s->timers[m]);
        i = m;
    }
    timer_set(s, i, t);
}

void kl_timer_add(TimerNode *t)
{
    Scheduler *s = kl_sched();
    ASSERT(t->idx < 0);
    if (s->ntimers == s->cap_timers) {
        int cap = s->cap_timers ? s->cap_timers * 2 : 16;
        TimerNode **arr = realloc(s->timers, sizeof(TimerNode *) * cap);
        ASSERT(arr);
        s->timers = arr;
        s->cap_timers = cap;
    }
    timer_set(s, s->ntimers++, t);
    timer_up(s, s->ntimers - 1);
}

void kl_timer_remove(TimerNode *t)
{
    if (t->idx < 0) return;
    Scheduler *s = kl_sched();
    int i = t->idx;
    t->idx = -1;
    int last = --s->ntimers;
    if (i < last) {
        timer_set(s, i, s->timers[last]);
        timer_down(s, i);
        timer_up(s, i);
    }
}

static TimerNode *timer_pop_min(Scheduler *s)
{
    TimerNode *t = s->timers[0];
    t->idx = -1;
    int last = --s->ntimers;
    if (last > 0) {
        timer_set(s, 0, s->timers[last]);
        timer_down(s, 0);
    }
    return t;
}

static void wait_timer_fire(void *arg)
{
    WaitRecord *w = arg;
    KoalaState *ks = w->ks;
    /* already woken by someone else */
    if (ks->state != KS_PARKED || ks->wait != w) return;
    w->result = KL_PARK_TIMEOUT;
    kl_ready(ks);
}

void kl_wait_init(WaitRecord *w, int kind)
{
    w->ks = __ks();
    w->kind = kind;
    w->result = KL_PARK_OK;
    w->value = nil_value;
    init_list(&w->link);
    w->timer.deadline = 0;
    w->timer.idx = -1;
    w->timer.fire = wait_timer_fire;
    w->timer.arg = w;
    w->timer.root = NULL;
}

/*---------------------------------------------------------------------------+
 |   Switching                                                               |
 +---------------------------------------------------------------------------*/

static void switch_to(KoalaState *next)
{
    KoalaState *cur = __ks();
    Scheduler *s = cur->sched;
    next->state = KS_RUNNING;
    /* updated before the switch: code on the new stack reads __ks() at once */
    __ts->current = next;
    s->switches++;
#if defined(__SANITIZE_ADDRESS__)
    void *fake_stack = NULL;
    Region *nr = next->region;
    __sanitizer_start_switch_fiber(&fake_stack, nr ? nr->lo + nr->guard : NULL,
                                   nr ? nr->c_size : 0);
#endif
    kl_fiber_switch(&cur->ctx, &next->ctx);
#if defined(__SANITIZE_ADDRESS__)
    __sanitizer_finish_switch_fiber(fake_stack, NULL, NULL);
#endif
}

/* A coroutine can't free the stack it runs on: the next park recycles the
 * regions of finished coroutines. */
static void recycle_dead(Scheduler *s)
{
    List *link;
    while ((link = list_pop_front(&s->dead))) {
        KoalaState *ks = list_entry(link, KoalaState, sched_link);
        if (ks->task) ks->task->ks = NULL;
        kl_abandon_ks(ks);
    }
}

static void deadlock(Scheduler *s)
{
    fprintf(stderr, "fatal: all tasks are parked (deadlock)\n");
    KoalaState *ks;
    list_foreach(ks, all_link, &s->all) {
        fprintf(stderr, "  coroutine %p: state %d, wait kind %d\n", (void *)ks, ks->state,
                ks->wait ? ks->wait->kind : -1);
    }
    exit(1);
}

/* Pick the next runnable coroutine; with none, wait for the earliest timer.
 * M1 has no poller: timers are the only wake source while idle. */
static KoalaState *pick_next(Scheduler *s)
{
    for (;;) {
        List *link = list_pop_front(&s->runq);
        if (link) return list_entry(link, KoalaState, sched_link);
        if (s->ntimers == 0) deadlock(s);
        int64_t now = kl_now_ns();
        int64_t dl = s->timers[0]->deadline;
        if (dl > now) {
            int64_t diff = dl - now;
            struct timespec req;
            req.tv_sec = diff / 1000000000LL;
            req.tv_nsec = diff % 1000000000LL;
            nanosleep(&req, NULL); /* EINTR: the loop rechecks the deadline */
        }
        while (s->ntimers > 0 && s->timers[0]->deadline <= kl_now_ns()) {
            TimerNode *t = timer_pop_min(s);
            t->fire(t->arg);
        }
    }
}

/*---------------------------------------------------------------------------+
 |   Core API                                                                |
 +---------------------------------------------------------------------------*/

void kl_ready(KoalaState *ks)
{
    Scheduler *s = ks->sched;
    if (ks->state == KS_PARKED) {
        WaitRecord *w = ks->wait;
        if (w) {
            if (!list_empty(&w->link)) list_remove(&w->link);
            kl_timer_remove(&w->timer);
            /* ks->wait stays until the coroutine resumes: the GC must still
             * see w->value while the result sits only in the C frame */
        }
    } else {
        ASSERT(ks->state == KS_RUNNABLE);
        /* already queued (e.g. woken twice): never enqueue twice */
        if (!list_empty(&ks->sched_link)) return;
    }
    ks->state = KS_RUNNABLE;
    list_push_back(&s->runq, &ks->sched_link);
}

int kl_park(WaitRecord *w)
{
    KoalaState *cur = __ks();
    Scheduler *s = cur->sched;
    ASSERT(w->ks == cur);

    /* cancellation requested while runnable: delivered at this park */
    if (cur->cancel) {
        cur->cancel = 0;
        cur->cancel_delivered = 1;
        return KL_PARK_CANCELLED;
    }

    cur->state = KS_PARKED;
    cur->wait = w;

    /* GC at park points only (plan §8.4a): every ks has a valid stack_top */
    if (s->gc_stress_park || gc_get_count() >= s->gc_park_threshold) gc_collect();

    KoalaState *next = pick_next(s);
    switch_to(next);

    /* resumed */
    recycle_dead(s);
    cur->wait = NULL;
    if (w->result == KL_PARK_CANCELLED) cur->cancel_delivered = 1;
    return w->result;
}

void kl_yield(void)
{
    KoalaState *cur = __ks();
    Scheduler *s = cur->sched;
    list_push_back(&s->runq, &cur->sched_link);
    List *link = list_pop_front(&s->runq);
    KoalaState *next = list_entry(link, KoalaState, sched_link);
    if (next != cur) {
        cur->state = KS_RUNNABLE;
        switch_to(next);
    }
    recycle_dead(s);
}

/* fiber entry: run the function, store the result, wake waiters, die */
static void kl_co_main(void *arg)
{
    KoalaState *ks = arg;
    Scheduler *s = ks->sched;
    TaskObject *task = ks->task;

    ks->started = 1;
    if (task) task->state = TASK_RUNNING;

    TValue ret = kl_call_code(ks->co_fn, ks->co_args, ks->co_nargs);

    /* entry args are shared with task->args; free them once */
    if (ks->co_args) mm_free(ks->co_args);
    ks->co_args = NULL;
    ks->co_fn = NULL;
    ks->co_nargs = 0;

    if (task) {
        task->fn = NULL;
        task->args = NULL;
        task->nargs = 0;
        if (is_error(&ret)) {
            Object *exc = pop_exc();
            if (ks->cancel_delivered && exc &&
                str_equal(kl_exc_get_msg(exc), "Cancelled")) {
                /* cancelled from outside: not a child failure (§6.7) */
                task->state = TASK_CANCELLED;
                task->result = nil_value;
                kl_free_exc(exc);
            } else {
                task->state = TASK_FAILED;
                task->exc = exc;
            }
        } else {
            task->state = TASK_DONE;
            task->result = ret;
        }
        kl_task_wake_joiners(task);
        if (task->group) kl_group_child_done(task->group, task);
    } else if (is_error(&ret)) {
        Object *exc = pop_exc();
        if (!(ks->cancel_delivered && exc && str_equal(kl_exc_get_msg(exc), "Cancelled"))) {
            print_exc_and_free(exc);
        } else {
            kl_free_exc(exc);
        }
    }

    ks->state = KS_DEAD;
    list_remove(&ks->all_link);
    list_push_back(&s->dead, &ks->sched_link);
    s->nlive--;

    /* switch away and never return */
    KoalaState *next = pick_next(s);
    switch_to(next);
    panic("fiber: dead coroutine resumed");
}

KoalaState *kl_spawn(Object *fn, TValue *args, int nargs)
{
    Scheduler *s = kl_sched();
    Region *r = kl_region_get();
    if (!r) panic("fiber: out of memory for a new coroutine region");
    KoalaState *ks = kl_new_co_ks(r);
    ks->co_fn = fn;
    if (nargs > 0) {
        ks->co_args = mm_alloc(sizeof(TValue) * nargs);
        memcpy(ks->co_args, args, sizeof(TValue) * nargs);
    }
    ks->co_nargs = nargs;
    kl_fiber_init(&ks->ctx, r->mid, kl_co_main, ks);
    list_push_back(&s->all, &ks->all_link);
    s->nlive++;
    s->spawned++;
    if (s->nlive > s->peak_live) s->peak_live = s->nlive;
    kl_ready(ks);
    return ks;
}

void kl_cancel_ks(KoalaState *ks)
{
    if (!ks || ks->state == KS_DEAD) return;
    Scheduler *s = ks->sched;

    if (ks->state == KS_PARKED) {
        WaitRecord *w = ks->wait;
        if (w) w->result = KL_PARK_CANCELLED;
        kl_ready(ks);
        return;
    }

    if (!ks->started) {
        /* never runs: drop it from the run queue and finish it now */
        list_remove(&ks->sched_link);
        ks->state = KS_DEAD;
        list_remove(&ks->all_link);
        list_push_back(&s->dead, &ks->sched_link);
        s->nlive--;
        TaskObject *task = ks->task;
        if (task) {
            task->state = TASK_CANCELLED;
            task->result = nil_value;
            kl_task_wake_joiners(task);
            if (task->group) kl_group_child_done(task->group, task);
        }
        return;
    }

    /* runnable (yielded): delivered at its next park, edge-triggered */
    ks->cancel = 1;
}

/*---------------------------------------------------------------------------+
 |   Lifetime                                                                |
 +---------------------------------------------------------------------------*/

Scheduler *kl_sched_new(KoalaState *main)
{
    Scheduler *s = mm_alloc_obj(s);
    s->main = main;
    init_list(&s->runq);
    init_list(&s->all);
    init_list(&s->dead);
    init_list(&s->tasks);
    main->sched = s;
    main->state = KS_RUNNING;
    init_list(&main->sched_link);
    init_list(&main->all_link);
    list_push_back(&s->all, &main->all_link);
    s->nlive = 1;
    s->peak_live = 1;
    const char *stress = getenv("KOALA_GC_STRESS");
    s->gc_stress_park = stress && str_equal(stress, "park");
    const char *gp = getenv("KOALA_GC_PARK");
    s->gc_park_threshold = gp ? atoll(gp) : 100000;
    return s;
}

void kl_sched_abandon(Scheduler *s)
{
    if (!s) return;

    /* unjoined task errors: print them at program exit (plan §6.10) */
    TaskObject *t;
    list_foreach(t, task_link, &s->tasks) {
        if (t->state == TASK_FAILED && !t->joined && t->exc) {
            print_exc_and_free(t->exc);
            t->exc = NULL;
            t->joined = 1;
        }
    }

    /* coroutines still parked (or never started) are abandoned without running */
    KoalaState *ks, *nxt;
    list_foreach_safe(ks, nxt, all_link, &s->all) {
        if (ks == s->main) continue;
        list_remove(&ks->all_link);
        list_remove(&ks->sched_link);
        if (ks->task) ks->task->ks = NULL;
        kl_abandon_ks(ks);
    }
    s->nlive = 1;

    /* finished coroutines whose region was not recycled yet */
    List *link;
    while ((link = list_pop_front(&s->dead))) {
        KoalaState *d = list_entry(link, KoalaState, sched_link);
        if (d->task) d->task->ks = NULL;
        kl_abandon_ks(d);
    }

    for (int i = 0; i < s->ntimers; i++) s->timers[i]->idx = -1;
    s->ntimers = 0;

    if (getenv("KOALA_CO_STATS")) {
        fprintf(stderr, "co: spawned %llu, peak live %d, switches %llu\n",
                (unsigned long long)s->spawned, s->peak_live, (unsigned long long)s->switches);
    }

    kl_region_pool_fini();
}

void kl_sched_destroy(Scheduler *s)
{
    if (!s) return;
    free(s->timers);
    mm_free(s);
}

#ifdef __cplusplus
}
#endif

