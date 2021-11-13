/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "timer.h"
#include <assert.h>
#include <pthread.h>
#include "util/common.h"

#ifdef __cplusplus
extern "C" {
#endif

static pthread_mutex_t timer_lock;
static BinHeap timer_heap;
static uint64_t timer_trigger;

static int timer_cmp_func(task_timer_t *p, task_timer_t *c)
{
    /* min binary heap */
    return p->tmo < c->tmo;
}

void init_timer(void)
{
    binheap_init(&timer_heap, 0, (BinHeapCmpFunc)timer_cmp_func);
    pthread_mutex_init(&timer_lock, NULL);
}

void fini_timer(void)
{
    assert(!binheap_top(&timer_heap));
    binheap_fini(&timer_heap);
}

void timer_start(task_timer_t *tm, uint64_t tmo, timer_func_t func, void *arg)
{
    assert(!tm->entry.idx);
    tm->func = func;
    tm->tmo = timer_trigger + (tmo / TIME_RESOLUTION_MS);
    tm->arg = arg;
    pthread_mutex_lock(&timer_lock);
    binheap_insert(&timer_heap, &tm->entry);
    pthread_mutex_unlock(&timer_lock);
}

void timer_stop(task_timer_t *tm)
{
    assert(tm->entry.idx);
    pthread_mutex_lock(&timer_lock);
    binheap_delete(&timer_heap, &tm->entry);
    pthread_mutex_unlock(&timer_lock);
}

void timer_poll(uint64_t trigger)
{
    timer_trigger += trigger;
    BinHeapEntry *e = binheap_top(&timer_heap);
    if (!e) return;
    task_timer_t *tm = CONTAINER_OF(e, task_timer_t, entry);
    while (tm->tmo <= timer_trigger) {
        binheap_delete(&timer_heap, e);
        tm->func(tm);
        e = binheap_top(&timer_heap);
        if (!e) break;
        tm = CONTAINER_OF(e, task_timer_t, entry);
    }
}

#ifdef __cplusplus
}
#endif
