/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_WSDEQUE_H_
#define _KOALA_WSDEQUE_H_

#include <stdatomic.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Chase-Lev work-stealing deque.
 *
 * D. Chase, Y. Lev, "Dynamic Circular Work-Stealing Deque", SPAA 2005.
 * N. M. Le, A. Pop, A. Cohen, F. Z. Nardelli, "Correct and Efficient
 * Work-Stealing for Weak Memory Models", PPoPP 2013.
 *
 * The owner thread calls push/pop at the bottom (LIFO), any other thread
 * calls steal at the top (FIFO). Items must not be NULL.
 */

typedef struct _WSArray {
    struct _WSArray *next; /* retired arrays, freed in fini */
    int64_t size;          /* power of two */
    _Atomic(void *) buf[];
} WSArray;

typedef struct _WSDeque {
    _Alignas(64) _Atomic int64_t top;
    _Alignas(64) _Atomic int64_t bottom;
    _Atomic(WSArray *) array;
    WSArray *retired;
} WSDeque;

typedef enum _WSStatus {
    WS_OK,
    WS_EMPTY,
    WS_ABORT, /* lost a race with another thief or the owner, retry */
} WSStatus;

void wsdq_init(WSDeque *dq, int64_t size);
void wsdq_fini(WSDeque *dq);

/* owner only */
void wsdq_push(WSDeque *dq, void *item);
void *wsdq_pop(WSDeque *dq);

/* any thread */
WSStatus wsdq_steal(WSDeque *dq, void **item);

static inline int64_t wsdq_size(WSDeque *dq)
{
    int64_t b = atomic_load_explicit(&dq->bottom, memory_order_relaxed);
    int64_t t = atomic_load_explicit(&dq->top, memory_order_relaxed);
    return b > t ? b - t : 0;
}

static inline int wsdq_empty(WSDeque *dq) { return wsdq_size(dq) == 0; }

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_WSDEQUE_H_ */
