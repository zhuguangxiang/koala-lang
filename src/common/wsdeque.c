/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "wsdeque.h"
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WS_RELAXED memory_order_relaxed
#define WS_ACQUIRE memory_order_acquire
#define WS_RELEASE memory_order_release
#define WS_SEQ_CST memory_order_seq_cst

static WSArray *ws_array_new(int64_t size)
{
    size_t bytes = sizeof(WSArray) + (size_t)size * sizeof(_Atomic(void *));
    WSArray *a = calloc(1, bytes);
    if (!a) {
        fprintf(stderr, "wsdeque: calloc %zu bytes failed\n", bytes);
        abort();
    }
    a->size = size;
    return a;
}

static inline void *ws_get(WSArray *a, int64_t i)
{
    return atomic_load_explicit(&a->buf[i & (a->size - 1)], WS_RELAXED);
}

static inline void ws_put(WSArray *a, int64_t i, void *item)
{
    atomic_store_explicit(&a->buf[i & (a->size - 1)], item, WS_RELAXED);
}

void wsdq_init(WSDeque *dq, int64_t size)
{
    int64_t n = 1;
    while (n < size) n <<= 1;
    atomic_init(&dq->top, 0);
    atomic_init(&dq->bottom, 0);
    atomic_init(&dq->array, ws_array_new(n));
    dq->retired = NULL;
}

void wsdq_fini(WSDeque *dq)
{
    free(atomic_load_explicit(&dq->array, WS_RELAXED));
    WSArray *a = dq->retired;
    while (a) {
        WSArray *nxt = a->next;
        free(a);
        a = nxt;
    }
    dq->retired = NULL;
}

/* thieves may still read the old array, so it is retired, not freed */
static WSArray *ws_grow(WSDeque *dq, WSArray *a, int64_t t, int64_t b)
{
    WSArray *na = ws_array_new(a->size << 1);
    for (int64_t i = t; i < b; i++) ws_put(na, i, ws_get(a, i));
    atomic_store_explicit(&dq->array, na, WS_RELEASE);
    a->next = dq->retired;
    dq->retired = a;
    return na;
}

void wsdq_push(WSDeque *dq, void *item)
{
    ASSERT(item);
    int64_t b = atomic_load_explicit(&dq->bottom, WS_RELAXED);
    int64_t t = atomic_load_explicit(&dq->top, WS_ACQUIRE);
    WSArray *a = atomic_load_explicit(&dq->array, WS_RELAXED);
    if (b - t > a->size - 1) a = ws_grow(dq, a, t, b);
    ws_put(a, b, item);
    atomic_thread_fence(WS_RELEASE);
    atomic_store_explicit(&dq->bottom, b + 1, WS_RELAXED);
}

void *wsdq_pop(WSDeque *dq)
{
    int64_t b = atomic_load_explicit(&dq->bottom, WS_RELAXED) - 1;
    WSArray *a = atomic_load_explicit(&dq->array, WS_RELAXED);
    atomic_store_explicit(&dq->bottom, b, WS_RELAXED);
    atomic_thread_fence(WS_SEQ_CST);
    int64_t t = atomic_load_explicit(&dq->top, WS_RELAXED);

    if (t > b) {
        /* empty */
        atomic_store_explicit(&dq->bottom, b + 1, WS_RELAXED);
        return NULL;
    }

    void *item = ws_get(a, b);
    if (t == b) {
        /* last item, race against thieves */
        if (!atomic_compare_exchange_strong_explicit(&dq->top, &t, t + 1, WS_SEQ_CST, WS_RELAXED)) {
            item = NULL;
        }
        atomic_store_explicit(&dq->bottom, b + 1, WS_RELAXED);
    }
    return item;
}

WSStatus wsdq_steal(WSDeque *dq, void **item)
{
    int64_t t = atomic_load_explicit(&dq->top, WS_ACQUIRE);
    atomic_thread_fence(WS_SEQ_CST);
    int64_t b = atomic_load_explicit(&dq->bottom, WS_ACQUIRE);

    if (t >= b) return WS_EMPTY;

    WSArray *a = atomic_load_explicit(&dq->array, WS_ACQUIRE);
    void *x = ws_get(a, t);
    if (!atomic_compare_exchange_strong_explicit(&dq->top, &t, t + 1, WS_SEQ_CST, WS_RELAXED)) {
        return WS_ABORT;
    }
    *item = x;
    return WS_OK;
}

#ifdef __cplusplus
}
#endif
