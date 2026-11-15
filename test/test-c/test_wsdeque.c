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

    test_report();
    return 0;
}
