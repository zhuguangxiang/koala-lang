/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <pthread.h>
#include "testhelp.h"
#include "wsdeque.h"

#define ITEM(i)  ((void *)(intptr_t)((i) + 1))
#define INDEX(p) ((intptr_t)(p) - 1)

#define NR_ITEMS   200000
#define NR_THIEVES 4

static WSDeque stress_dq;
static _Atomic int seen[NR_ITEMS];
static atomic_int owner_done;

static void take(void *p) { atomic_fetch_add(&seen[INDEX(p)], 1); }

static void *thief(void *arg)
{
    (void)arg;
    void *p;
    for (;;) {
        WSStatus s = wsdq_steal(&stress_dq, &p);
        if (s == WS_OK) {
            take(p);
        } else if (s == WS_EMPTY && atomic_load(&owner_done)) {
            break;
        }
    }
    return NULL;
}

static int stress(void)
{
    wsdq_init(&stress_dq, 4);
    pthread_t tids[NR_THIEVES];
    for (int i = 0; i < NR_THIEVES; i++) pthread_create(&tids[i], NULL, thief, NULL);

    /* the owner interleaves pushes and pops, forcing growth and races */
    for (int i = 0; i < NR_ITEMS; i++) {
        wsdq_push(&stress_dq, ITEM(i));
        if (i % 3 == 0) {
            void *p = wsdq_pop(&stress_dq);
            if (p) take(p);
        }
    }
    void *p;
    while ((p = wsdq_pop(&stress_dq))) take(p);
    atomic_store(&owner_done, 1);

    for (int i = 0; i < NR_THIEVES; i++) pthread_join(tids[i], NULL);
    wsdq_fini(&stress_dq);

    for (int i = 0; i < NR_ITEMS; i++) {
        if (atomic_load(&seen[i]) != 1) return 0;
    }
    return 1;
}

/*
 * Several owners, each with its own deque, growing at the same time while thieves steal
 * from all of them. This is the parallel-marking shape: one deque per marker thread.
 * Run under tsan_wsdeque.sh it also catches an allocator that is not thread-safe
 * (mm_alloc with MM_TRACK_BLOCKS was).
 */

#define NR_OWNERS       4
#define NR_OWNER_ITEMS  100000
#define NR_ALL_ITEMS    (NR_OWNERS * NR_OWNER_ITEMS)
#define NR_OWNER_THIEVES 4

static WSDeque owner_dqs[NR_OWNERS];
static _Atomic int owner_seen[NR_ALL_ITEMS];
static atomic_int owners_done;

static void owner_take(void *p) { atomic_fetch_add(&owner_seen[INDEX(p)], 1); }

static void *owner(void *arg)
{
    int o = (int)(intptr_t)arg;
    WSDeque *dq = &owner_dqs[o];
    int base = o * NR_OWNER_ITEMS;

    /* pop rarely, so each deque grows from 4 to 2^17 while the others grow too */
    for (int i = 0; i < NR_OWNER_ITEMS; i++) {
        wsdq_push(dq, ITEM(base + i));
        if (i % 7 == 0) {
            void *p = wsdq_pop(dq);
            if (p) owner_take(p);
        }
    }
    void *p;
    while ((p = wsdq_pop(dq))) owner_take(p);
    atomic_fetch_add(&owners_done, 1);
    return NULL;
}

static void *owner_thief(void *arg)
{
    int start = (int)(intptr_t)arg;
    for (;;) {
        /* once every owner is done nothing is pushed again, so one quiet pass means empty */
        int done = atomic_load(&owners_done) == NR_OWNERS;
        int quiet = 1;
        for (int k = 0; k < NR_OWNERS; k++) {
            void *p;
            WSStatus s = wsdq_steal(&owner_dqs[(start + k) % NR_OWNERS], &p);
            if (s == WS_OK) {
                owner_take(p);
                quiet = 0;
            } else if (s == WS_ABORT) {
                quiet = 0;
            }
        }
        if (done && quiet) break;
    }
    return NULL;
}

static int multi_owner(void)
{
    for (int o = 0; o < NR_OWNERS; o++) wsdq_init(&owner_dqs[o], 4);

    pthread_t thieves[NR_OWNER_THIEVES];
    pthread_t owners[NR_OWNERS];
    for (int i = 0; i < NR_OWNER_THIEVES; i++) {
        pthread_create(&thieves[i], NULL, owner_thief, (void *)(intptr_t)i);
    }
    for (int o = 0; o < NR_OWNERS; o++) {
        pthread_create(&owners[o], NULL, owner, (void *)(intptr_t)o);
    }
    for (int o = 0; o < NR_OWNERS; o++) pthread_join(owners[o], NULL);
    for (int i = 0; i < NR_OWNER_THIEVES; i++) pthread_join(thieves[i], NULL);

    int ok = 1;
    for (int o = 0; o < NR_OWNERS; o++) {
        if (!wsdq_empty(&owner_dqs[o])) ok = 0;
        wsdq_fini(&owner_dqs[o]);
    }
    for (int i = 0; i < NR_ALL_ITEMS; i++) {
        if (atomic_load(&owner_seen[i]) != 1) ok = 0;
    }
    return ok;
}

/*
 * Owners that create, grow and destroy their own deques over and over at the same time,
 * with no thieves: every init, grow and fini allocates or frees concurrently with the
 * other owners. Each round checks LIFO order.
 */

#define NR_CHURN_ROUNDS 200
#define NR_CHURN_ITEMS  1024

static atomic_int churn_bad;

static void *churner(void *arg)
{
    (void)arg;
    for (int r = 0; r < NR_CHURN_ROUNDS; r++) {
        WSDeque dq;
        wsdq_init(&dq, 4);
        for (int i = 0; i < NR_CHURN_ITEMS; i++) wsdq_push(&dq, ITEM(i));
        for (int i = NR_CHURN_ITEMS - 1; i >= 0; i--) {
            void *p = wsdq_pop(&dq);
            if (!p || INDEX(p) != i) atomic_store(&churn_bad, 1);
        }
        if (wsdq_pop(&dq) != NULL) atomic_store(&churn_bad, 1);
        wsdq_fini(&dq);
    }
    return NULL;
}

static int churn(void)
{
    pthread_t tids[NR_OWNERS];
    for (int i = 0; i < NR_OWNERS; i++) pthread_create(&tids[i], NULL, churner, NULL);
    for (int i = 0; i < NR_OWNERS; i++) pthread_join(tids[i], NULL);
    return !atomic_load(&churn_bad);
}

int main(void)
{
    WSDeque dq;
    void *p;
    wsdq_init(&dq, 4);

    test_cond("New deque is empty", wsdq_empty(&dq) && wsdq_pop(&dq) == NULL);
    test_cond("Steal from empty deque", wsdq_steal(&dq, &p) == WS_EMPTY);

    for (int i = 0; i < 10; i++) wsdq_push(&dq, ITEM(i));
    test_cond("Push past capacity grows", wsdq_size(&dq) == 10);

    int ok = 1;
    for (int i = 0; i < 3; i++) {
        if (wsdq_steal(&dq, &p) != WS_OK || INDEX(p) != i) ok = 0;
    }
    test_cond("Steal takes the top in FIFO order", ok);

    ok = 1;
    for (int i = 9; i >= 3; i--) {
        p = wsdq_pop(&dq);
        if (!p || INDEX(p) != i) ok = 0;
    }
    test_cond("Pop takes the bottom in LIFO order", ok && wsdq_empty(&dq));
    test_cond("Pop after drain returns NULL", wsdq_pop(&dq) == NULL);

    wsdq_fini(&dq);

    test_cond("Owner and thieves take every item exactly once", stress());
    test_cond("Several owners grow at once while thieves steal from all", multi_owner());
    test_cond("Several owners init, grow and fini their deques at once", churn());

    test_report();
    return 0;
}
