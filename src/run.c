/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "run.h"
#include <unistd.h>
#include "eval.h"
#include "log.h"
#include "mm.h"
#include "tracestack.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------DATA-----------------------------------*/

/* pthread */
static int __nthreads;
static ThreadState *_threads;
__thread ThreadState *__ts;

/* all done KoalaState list */
static LLDeque _gs_done_list;
/* global running KoalaState list */
static LLDeque _gs_run_list;

/* arguments from prompt */
static int _gs_argc;
static char **_gs_argv;

/* system state */
static volatile int _gs_done;

/* waiting for available KoalaState */
static pthread_mutex_t _gs_mutex;
static pthread_cond_t _gs_cond;

/* all loaded modules(dict) */
static Object *_gs_modules;

/*-------------------------------------API-----------------------------------*/

void kl_run_ks(KoalaState *ks);

void schedule(void) {}

void yield(void) {}

void suspend(int timeout) {}

void resume(KoalaState *ks) {}

static void *koala_pthread_func(void *arg)
{
    ThreadState *ts = arg;
    __ts = ts;

    while (ts->state == TS_RUNNING) {
        KoalaState *ks = ts->current;
        if (!ks) {
            /* get from local list */
            LLDqNode *node = lldq_pop_head(&ts->run_list);
            if (!node) {
                /* steal from other threads */
                node = NULL; // steal_one_ks(ts);
            }

            if (!node) {
                /* get from global list */
                node = lldq_pop_head(&_gs_run_list);
            }

            if (node) {
                /* got one thread */
                ks = CONTAINER_OF(node, KoalaState, link);
                ts->current = ks;
                continue;
            }

            /* suspend at global list */
            pthread_mutex_lock(&_gs_mutex);
            log_info("Thread-%d is suspended.", ts->id);
            ts->state = TS_WAIT;
            int empty = lldq_empty(&_gs_run_list);
            while (empty && ts->state != TS_DONE) {
                pthread_cond_wait(&_gs_cond, &_gs_mutex);
                empty = lldq_empty(&_gs_run_list);
            }
            pthread_mutex_unlock(&_gs_mutex);

            if (ts->state != TS_DONE) {
                /* again, get from global list, high successfully */
                node = lldq_pop_head(&_gs_run_list);
                if (node) {
                    /* got one thread */
                    ks = CONTAINER_OF(node, KoalaState, link);
                    ts->current = ks;
                }
                ts->state = TS_RUNNING;
                log_info("Thread-%d is running.", ts->id);
            }
        } else {
            kl_run_ks(ks);
        }
    }

    log_info("Thread-%d is done.", ts->id);
}

static void init_threads(int nthreads)
{
    /* initialize koala threads */
    __nthreads = nthreads;
    _threads = mm_alloc(sizeof(ThreadState) * nthreads);
    ASSERT(_threads);

    /* initialize main thread as koala thread */
    ThreadState *ts = _threads;
    lldq_init(&ts->run_list);
    ts->current = ks_new();
    ts->id = 1;
    ts->steal_count = 0;
    ts->state = TS_RUNNING;
    __ts = ts;

    /* initialize other koala threads */
    for (int i = 1; i < nthreads; i++) {
        ts = _threads + i;
        lldq_init(&ts->run_list);
        ts->current = NULL;
        ts->id = i + 1;
        ts->steal_count = 0;
        ts->state = TS_RUNNING;
        int ret = pthread_create(&ts->pid, NULL, koala_pthread_func, ts);
        ASSERT(!ret);
    }
}

void init_bltin_module(KoalaState *ks);
void init_sys_module(KoalaState *ks);

void kl_init(int argc, char *argv[])
{
    /* init garbage collection */
    init_gc_system(MAX_GC_MEM_SIZE, 0.8f);

    /* init global mutex&cond */
    pthread_mutex_init(&_gs_mutex, NULL);
    pthread_cond_init(&_gs_cond, NULL);

    /* init global koala state list */
    lldq_init(&_gs_done_list);
    lldq_init(&_gs_run_list);
    _gs_argc = argc;
    _gs_argv = argv;

    /* init koala threads */
    init_threads(1);

    /* init koala builtin & sys module */
    KoalaState *ks = __ks();
    // init_bltin_module(ks);
    // init_sys_module(ks);
}

static int done(void)
{
    int done = 1;

    // check all threads are in suspend state
    for (int i = 1; i < __nthreads; i++) {
        ThreadState *ts = _threads + i;
        if (ts->state != TS_WAIT) {
            done = 0;
            break;
        }
    }

    if (done) {
        // signal to all suspended pthread
        for (int i = 1; i < __nthreads; i++) {
            ThreadState *ts = _threads + i;
            ts->state = TS_DONE;
        }
        pthread_mutex_lock(&_gs_mutex);
        pthread_cond_broadcast(&_gs_cond);
        pthread_mutex_unlock(&_gs_mutex);
    }

    return done;
}

static void clear_done_state(void)
{
    LLDqNode *node = lldq_pop_head(&_gs_done_list);
    while (node) {
        KoalaState *ks = CONTAINER_OF(node, KoalaState, link);
        ks_free(ks);
        node = lldq_pop_head(&_gs_done_list);
    }
}

/* monitor */
void kl_run_file(const char *filename)
{
    /* load klc and run */

    while (1) {
        /* monitor koala threads */
        if (done()) break;

        clear_done_state();
        // sleep(1);
    }

    /* join koala threads */
    for (int i = 1; i < __nthreads; i++) {
        ThreadState *ts = _threads + i;
        ASSERT(ts->state == TS_DONE);
        pthread_join(ts->pid, NULL);
    }
}

void kl_fini(void)
{
    // signal to all suspended pthread
    pthread_mutex_lock(&_gs_mutex);
    for (int i = 1; i < __nthreads; i++) {
        ThreadState *ts = _threads + i;
        ts->state = TS_DONE;
    }
    pthread_cond_broadcast(&_gs_cond);
    pthread_mutex_unlock(&_gs_mutex);

    /* join koala threads */
    for (int i = 1; i < __nthreads; i++) {
        ThreadState *ts = _threads + i;
        pthread_join(ts->pid, NULL);
        ASSERT(ts->state == TS_DONE);
        ks_free(ts->current);
    }

    ASSERT(lldq_empty(&_gs_run_list));
    ASSERT(lldq_empty(&_gs_done_list));

    ks_free(__ts->current);
    mm_free(_threads);

    fini_gc_system();
}

void kl_run_ks(KoalaState *ks) {}

int check_all_threads_stw(void)
{
    int yes = 1;

    for (int i = 1; i < __nthreads; i++) {
        ThreadState *ts = _threads + i;
        if (ts->state != TS_GC_STW) {
            yes = 0;
            break;
        }
    }

    return yes;
}

static void enum_koala_state(Queue *que, KoalaState *ks)
{
    if (!ks) return;

    CallFrame *cf = ks->cf;
    while (cf) {
        int size = cf->local_size + cf->stack_size;
        Value *v;
        for (int i = 0; i < size; i++) {
            v = cf->local_stack + i;
            gc_mark_value(v, que);
        }
        cf = cf->back;
    }

    /* enumerate trace shadow stack */
    TraceStack *trace = ks->trace_stacks;
    while (trace) {
        void **obj_p;
        for (int i = 0; i < trace->avail; i++) {
            obj_p = trace->obj_p[i];
            gc_mark_obj(*obj_p, que);
        }
        trace = trace->back;
    }
}

int enum_all_roots(Queue *que)
{
    /* enum global running list */
    KoalaState *ks;
    lldq_foreach(ks, link, &_gs_run_list) {
        enum_koala_state(que, ks);
    }

    /* enum local running lists */
    for (int i = 0; i < __nthreads; i++) {
        ThreadState *ts = _threads + i;
        enum_koala_state(que, ts->current);
        lldq_foreach(ks, link, &ts->run_list) {
            enum_koala_state(que, ks);
        }
    }

    /* enum global variables */

    return 0;
}

#ifdef __cplusplus
}
#endif
