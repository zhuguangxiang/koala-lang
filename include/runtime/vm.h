/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_VM_H_
#define _KOALA_VM_H_

#include "fiber.h"
#include "list.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
struct _KoalaState;
struct _ThreadState;
struct _Scheduler;
struct _TaskObject;
struct _WaitRecord;
struct _ShadowStack;

/* call stack frame information */
typedef struct _CallFrame {
    /* call stack back frame */
    struct _CallFrame *back;

    /* point back to KoalaState */
    struct _KoalaState *ks;

    /* code for this call */
    CodeObject *code;

    /* module */
    Object *module;

    /* program counter for this call */
    uint32_t pc;

    /* number of locals(include parameters) */
    int nlocals;

    /* locals */
    TValue *locals;

    /* frames replaced by tail calls to other functions, for traceback */
    int tailcalls;
} CallFrame;

/* per koala thread */
typedef struct _KoalaState {
    /* top call stack frame */
    CallFrame *cf;
    /* depth of call frames */
    int depth;

    /* stack size */
    int stack_size;
    /* base stack pointer */
    TValue *stack_base;

    /* stack top pointer */
    TValue *stack_top;
    /* stack_base + stack_size, read by JIT code to check the stack */
    TValue *stack_limit;
    /* lowest native stack address for interpreter and JIT calls */
    char *c_stack_limit;

    /* call frame cache */
    CallFrame *cf_cache;

    /* builtin module */
    Object *builtin;
    /* sys module */
    Object *sys;

    /* exception */
    Object *exc;

    /* point to _ThreadState */
    struct _ThreadState *ts;

    /* coroutine support (plan §5.6) */

    /* saved rsp while not running */
    FiberCtx ctx;
    /* NULL for the main ks (runs on the pthread stack) */
    Region *region;
    /* owning task, NULL for main and raw spawns */
    struct _TaskObject *task;
    struct _Scheduler *sched;
    /* run queue or a wait list */
    List sched_link;
    /* scheduler registry */
    List all_link;
    /* KS_RUNNABLE, KS_RUNNING, KS_PARKED, KS_DEAD (sched.h) */
    uint32_t state;
    /* cancellation requested, delivered at the next park */
    uint32_t cancel;
    /* a park returned KL_PARK_CANCELLED: a final Cancelled error classifies
     * the task as CANCELLED, not FAILED */
    uint32_t cancel_delivered;
    /* the entry function has started */
    uint32_t started;
    /* native temporary roots (shadowstack.h) */
    struct _ShadowStack *shadow;
    /* what this ks is parked on */
    struct _WaitRecord *wait;
    /* entry function and args, C-owned, freed when the coroutine finishes */
    Object *co_fn;
    TValue *co_args;
    int co_nargs;
} KoalaState;

/* per pthread, thread local storage */
typedef struct _ThreadState {
    /* current running KoalaState in this thread */
    KoalaState *current;
    /* the N:1 scheduler of this thread */
    struct _Scheduler *sched;
} ThreadState;

/* get current thread */
extern __thread ThreadState *__ts;

/* get current koala state  */
static inline KoalaState *__ks(void) { return __ts->current; }

KoalaState *kl_new_ks(void);
void kl_free_ks(KoalaState *ks);

/* Create a coroutine KoalaState on a fiber region: VM stack at the high end,
 * C stack below it. builtin/sys are shared with the main ks. */
KoalaState *kl_new_co_ks(Region *r);
/* Free a coroutine ks without the !cf assertion: parked at program exit or
 * finished and waiting for region recycling. Frees active frames, the frame
 * cache and returns the region to the pool. */
void kl_abandon_ks(KoalaState *ks);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_VM_H_ */
