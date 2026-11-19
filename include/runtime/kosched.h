/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_KOSCHED_H_
#define _KOALA_KOSCHED_H_

#include "fiber.h"
#include "list.h"
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Single-P (N:1) cooperative scheduler. One pthread runs every coroutine;
 * a switch happens only inside a native C call (a GC safepoint). There is
 * no scheduler fiber: kl_park picks the next runnable coroutine and
 * switches straight to it.
 */

/* KoalaState.state */
#define KS_RUNNABLE 0 /* on the run queue, not running */
#define KS_RUNNING  1 /* currently running */
#define KS_PARKED   2 /* waiting on a WaitRecord */
#define KS_DEAD     3 /* finished, region waiting to be recycled */

/* kl_park results */
#define KL_PARK_OK        0 /* woken normally, WaitRecord.value is valid */
#define KL_PARK_CANCELLED 1 /* cancelled while parked (or before a park) */
#define KL_PARK_TIMEOUT   2 /* a deadline timer fired */
#define KL_PARK_CLOSED    3 /* channel closed while parked */

/* WaitRecord.kind */
#define WAIT_TIMER      1 /* sleep / deadline */
#define WAIT_CHAN_SEND  2
#define WAIT_CHAN_RECV  3
#define WAIT_JOIN       4
#define WAIT_GROUP      5

/* A deadline in the scheduler's timer min-heap. `fire` runs on the parking
 * coroutine's stack when the deadline passes; `root` is an optional GC
 * reference the heap must keep alive (e.g. a Group with a deadline). */
typedef struct _TimerNode {
    int64_t deadline; /* CLOCK_MONOTONIC nanoseconds */
    int idx;          /* heap index, -1 when not queued */
    void (*fire)(void *arg);
    void *arg;
    Object *root;
} TimerNode;

/* Lives in the parked native's C frame, reachable through ks->wait so the
 * GC marks `value`. Nobody writes a parked coroutine's VM stack: results go
 * here and the coroutine copies them out after waking. */
typedef struct _WaitRecord {
    KoalaState *ks;
    int kind;
    int result; /* KL_PARK_* */
    TValue value;
    /* on the wait queue of the object being waited for, self-linked otherwise */
    List link;
    TimerNode timer;
} WaitRecord;

typedef struct _Scheduler {
    /* root coroutine: the pthread's original ks */
    KoalaState *main;
    /* KoalaState.sched_link, FIFO */
    List runq;
    /* KoalaState.all_link, every live ks (GC roots) */
    List all;
    /* finished ks whose region can be recycled */
    List dead;
    /* TaskObject.task_link, every live task (exit-time error reporting) */
    List tasks;
    int nlive;
    /* timer min-heap keyed by deadline */
    TimerNode **timers;
    int ntimers;
    int cap_timers;
    /* stats */
    uint64_t switches;
    uint64_t spawned;
    int peak_live;
    /* KOALA_GC_STRESS=park: collect on every park */
    int gc_stress_park;
    /* collect at a park when gc_get_count() reaches this (KOALA_GC_PARK) */
    int64_t gc_park_threshold;
} Scheduler;

/* Create the scheduler and register `main` as the root coroutine. */
Scheduler *kl_sched_new(KoalaState *main);
/* Abandon coroutines still parked at program exit and report unjoined task
 * errors; runs before gc_fini. The Scheduler itself stays valid until
 * kl_sched_destroy, because TaskObject._free still uses it during gc_fini. */
void kl_sched_abandon(Scheduler *s);
void kl_sched_destroy(Scheduler *s);

/* Make ks runnable and append it to the run queue; never switches.
 * Removes ks from its wait queue and deadline timer when parked. */
void kl_ready(KoalaState *ks);

/* Suspend the current coroutine until someone calls kl_ready on it (or a
 * deadline fires, or it is cancelled). The caller links `w` into whatever
 * wait queue applies before calling; w->value carries the result. */
int kl_park(WaitRecord *w);

/* Give up the CPU; stay runnable. */
void kl_yield(void);

/* Create a coroutine running fn(args); runnable but not started. The args
 * are copied into a C-owned block freed when the coroutine finishes. */
KoalaState *kl_spawn(Object *fn, TValue *args, int nargs);

/* Request cancellation. Parked: woken with KL_PARK_CANCELLED. Not started:
 * never runs. Running/runnable: delivered at its next park (edge-triggered). */
void kl_cancel_ks(KoalaState *ks);

/* Initialize a wait record for the current coroutine. */
void kl_wait_init(WaitRecord *w, int kind);

/* Monotonic clock in nanoseconds. */
int64_t kl_now_ns(void);

/* Deadline timer heap. t->idx must be -1 before add; remove is a no-op
 * when not queued. */
void kl_timer_add(TimerNode *t);
void kl_timer_remove(TimerNode *t);

/* The scheduler of the current pthread. */
static inline Scheduler *kl_sched(void) { return __ts->sched; }

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_KOSCHED_H_ */
