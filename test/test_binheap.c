/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "util/binheap.h"
#include "util/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct timer {
    unsigned int at_time;
    BinHeapEntry entry;
};

struct timer test_timers[] = {
    { 26 }, { 35 }, { 12 }, { 20 }, { 5 },
    { 34 }, { 23 }, { 14 }, { 24 }, { 9 },
};

struct timer test_timers_sorted[] = {
    { 5 },  { 9 },  { 12 }, { 14 }, { 20 },
    { 23 }, { 24 }, { 26 }, { 34 }, { 35 },
};

struct timer test_timers_sorted_max[] = {
    { 35 }, { 34 }, { 26 }, { 24 }, { 23 },
    { 20 }, { 14 }, { 12 }, { 9 },  { 5 },
};

int timer_compare(BinHeapEntry *p, BinHeapEntry *c)
{
    struct timer *ta, *tb;
    ta = CONTAINER_OF(p, struct timer, entry);
    tb = CONTAINER_OF(c, struct timer, entry);
    // min heap
    // return ta->at_time < tb->at_time;
    // max heap
    return ta->at_time > tb->at_time;
}

void test_binheap(void)
{
    BinHeap heap;
    binheap_init(&heap, 0, timer_compare);

    struct timer *timer;
    int ret;
    for (int i = 0; i < COUNT_OF(test_timers); i++) {
        timer = test_timers + i;
        ret = binheap_insert(&heap, &timer->entry);
        assert(!ret);
        // printf("timer-%d:%ld\n", i, timer->entry.idx);
    }

    BinHeapEntry *e = binheap_next(&heap, null);
    printf("[");
    while (e) {
        timer = CONTAINER_OF(e, struct timer, entry);
        e = binheap_next(&heap, e);
        if (e)
            printf("%d ", timer->at_time);
        else
            printf("%d", timer->at_time);
    }
    printf("]\n");

    printf("extracted in order: ");
    e = binheap_pop(&heap);
    int i = 0;
    while (e) {
        timer = CONTAINER_OF(e, struct timer, entry);
        // min heap
        // assert(timer->at_time == test_timers_sorted[i].at_time);
        // max heap
        assert(timer->at_time == test_timers_sorted_max[i].at_time);
        printf("%d ", timer->at_time);
        e = binheap_pop(&heap);
        ++i;
    }
    printf("\n");

    binheap_fini(&heap);
}

int main(int argc, char *argv[])
{
    test_binheap();
    return 0;
}

#ifdef __cplusplus
}
#endif
