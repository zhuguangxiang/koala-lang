/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "run.h"
#include <unistd.h>
#include "eval.h"
#include "log.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------DATA-----------------------------------*/

/* pthread */
int __nthreads;
static ThreadState *_threads;
pthread_key_t __local_key;

/* all done KoalaState list */
static LLDeque _gc_done_list;
/* global running KoalaState list */
static LLDeque _gs_run_list;
/* arguments from prompt */
static int _gs_argc;
static const char **_gs_argv;

/* waiting for available KoalaState */
static pthread_mutex_t _gs_mutex;
static pthread_cond_t _gs_cond;

/*-------------------------------------API-----------------------------------*/

void kl_run_ks(KoalaState *ks);

void schedule(void) {}

void yield(void) {}

void suspend(int timeout) {}

void resume(KoalaState *ks) {}

static void *koala_pthread_func(void *arg)
{
    ThreadState *ts = arg;
    pthread_setspecific(__local_key, ts);

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
            log_info("Thread-%d is suspended.", ts->id);
            ts->state = TS_WAIT;
            pthread_mutex_lock(&_gs_mutex);
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

    /* simulate gc(later delete) */
    ts->state = TS_RUNNING;
    int i = 0;
    while (1) {
        // sleep(1);
        gc_check_stw();
        i++;
        log_info("Thread-%d is running %d", ts->id, i);
        if (ts->id == 1) {
            gc_alloc(50, NULL);
        } else if (ts->id == 2) {
            gc_alloc(80, NULL);
        }
    }
}

static void init_threads(int nthreads)
{
    /* initialize koala threads */
    __nthreads = nthreads;
    _threads = mm_alloc_fast(sizeof(ThreadState) * nthreads);
    ASSERT(_threads);

    for (int i = 0; i < nthreads; i++) {
        ThreadState *ts = _threads + i;
        lldq_init(&ts->run_list);
        ts->current = NULL;
        ts->id = i + 1;
        ts->steal_count = 0;
        ts->state = TS_RUNNING;
        int ret = pthread_create(&ts->pid, NULL, koala_pthread_func, ts);
        ASSERT(!ret);
    }
}

void kl_init(int argc, const char *argv[])
{
    /* init garbage collection */
    init_gc_system(MAX_GC_MEM_SIZE, 0.8f);

    /* init global mutex&cond */
    pthread_mutex_init(&_gs_mutex, NULL);
    pthread_cond_init(&_gs_cond, NULL);

    /* init global koala state list */
    lldq_init(&_gc_done_list);
    lldq_init(&_gs_run_list);
    _gs_argc = argc;
    _gs_argv = argv;

    /* monitor main thread */
    pthread_key_create(&__local_key, NULL);

    /* init koala threads */
    init_threads(3);

    /* init koala builtin module */
}

static int done(void)
{
    int done = 1;

    // check all threads are in suspend state
    for (int i = 0; i < __nthreads; i++) {
        ThreadState *ts = _threads + i;
        if (ts->state != TS_WAIT) {
            done = 0;
            break;
        }
    }

    if (done) {
        // signal to all suspended pthread
        for (int i = 0; i < __nthreads; i++) {
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
    LLDqNode *node = lldq_pop_head(&_gc_done_list);
    while (node) {
        KoalaState *ks = CONTAINER_OF(node, KoalaState, link);
        ks_free(ks);
        node = lldq_pop_head(&_gc_done_list);
    }
}

/* monitor */
void kl_run(const char *filename)
{
    /* load klc and run */

    while (1) {
        /* monitor koala threads */
        // if (done()) break;
        done();

        clear_done_state();
        sleep(1);
    }

    /* join koala threads */
    for (int i = 0; i < __nthreads; i++) {
        ThreadState *ts = _threads + i;
        ASSERT(ts->state == TS_DONE);
        pthread_join(ts->pid, NULL);
    }
}

void kl_fini(void) {}

void kl_run_ks(KoalaState *ks) {}

int check_all_threads_stw(void)
{
    int yes = 1;

    for (int i = 0; i < __nthreads; i++) {
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

    Value *v;
    Object *obj;
    for (int i = 0; i < ks->stack_size; i++) {
        v = ks->base_stack_ptr + i;
        if (v->tag & 0x1) {
            obj = v->obj;
            if (obj->ob_gchdr.age != -1) {
                gc_mark(obj, GC_COLOR_GRAY);
                queue_push(que, obj);
            }
        }
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
