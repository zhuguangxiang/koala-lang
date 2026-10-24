/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "ring.h"
#include "testhelp.h"

int ringTest(void)
{
    Ring ring;
    ring_init(&ring, sizeof(int));

    /* ---------- empty state ---------- */
    test_cond("Create an empty ring", ring_empty(&ring) && ring_size(&ring) == 0);
    test_cond("Pop from an empty ring fails", ring_pop_front(&ring, NULL) == -1);
    test_cond("Peek an empty ring returns NULL", ring_peek_front(&ring) == NULL);

    /* ---------- FIFO order ---------- */
    for (int i = 0; i < 10; ++i) ring_push_back(&ring, &i);
    test_cond("Push 10 objects into the ring", ring_size(&ring) == 10);

    int ok = 1;
    for (int i = 0; i < 10; ++i) {
        int v = -1;
        ring_pop_front(&ring, &v);
        if (v != i) ok = 0;
    }
    test_cond("Pop objects in FIFO order", ok && ring_empty(&ring));

    /* after 10 pushes the buffer holds 16 slots */
    test_cond("Capacity is a power of two", ring_capacity(&ring) == 16);

    /* ---------- wrap around ---------- */
    for (int i = 0; i < 8; ++i) ring_push_back(&ring, &i);
    /* head = 6, size = 2 */
    for (int i = 0; i < 6; ++i) ring_pop_front(&ring, NULL);
    /* the tail wraps back to slot 0 */
    for (int i = 8; i < 18; ++i) ring_push_back(&ring, &i);

    ok = 1;
    for (int i = 6; i < 18; ++i) {
        int v = -1;
        ring_pop_front(&ring, &v);
        if (v != i) ok = 0;
    }
    test_cond("Push after pop wraps around the buffer", ok && ring_empty(&ring));

    /* ---------- indexed access across the wrap ---------- */
    for (int i = 0; i < 8; ++i) ring_push_back(&ring, &i);
    /* head = 6, size = 2 */
    for (int i = 0; i < 6; ++i) ring_pop_front(&ring, NULL);
    /* size = 10, straddles the end of the buffer */
    for (int i = 8; i < 16; ++i) ring_push_back(&ring, &i);

    ok = 1;
    for (int i = 0; i < 10; ++i) {
        int *p = ring_get_ptr(&ring, i);
        if (p == NULL || *p != i + 6) ok = 0;
    }
    test_cond("Index objects from the front across the wrap", ok);
    test_cond("Peek front/back across the wrap",
              *(int *)ring_peek_front(&ring) == 6 && *(int *)ring_peek_back(&ring) == 15);

    /* ---------- both ends, deque semantics ---------- */
    ring_clear(&ring);
    int one = 1, two = 2, zero = 0;
    ring_push_back(&ring, &one);
    ring_push_back(&ring, &two);
    ring_push_front(&ring, &zero);
    test_cond("push_front prepends an object",
              ring_size(&ring) == 3 && *(int *)ring_peek_front(&ring) == 0);

    int out = -1;
    ring_pop_back(&ring, &out);
    test_cond("pop_back removes the tail", out == 2 && ring_size(&ring) == 2);

    ring_pop_front(&ring, &out);
    test_cond("pop_front removes the head", out == 0 && ring_size(&ring) == 1);

    ring_pop_front(&ring, &out);
    test_cond("Both ends keep the order", out == 1 && ring_empty(&ring));

    /* ---------- growth keeps order ---------- */
    ring_clear(&ring);
    for (int i = 0; i < 1000; ++i) ring_push_back(&ring, &i);

    ok = 1;
    for (int i = 0; i < 1000; ++i) {
        int v = -1;
        ring_pop_front(&ring, &v);
        if (v != i) ok = 0;
    }
    test_cond("Grow keeps FIFO order for 1000 objects", ok && ring_empty(&ring));

    /* ---------- growth while wrapped ---------- */
    ring_clear(&ring);
    for (int i = 0; i < 8; ++i) ring_push_back(&ring, &i);
    /* head = 6 */
    for (int i = 0; i < 6; ++i) ring_pop_front(&ring, NULL);
    /* grows several times while the live range is split in two pieces */
    for (int i = 8; i < 1000; ++i) ring_push_back(&ring, &i);

    ok = 1;
    for (int i = 6; i < 1000; ++i) {
        int v = -1;
        ring_pop_front(&ring, &v);
        if (v != i) ok = 0;
    }
    test_cond("Grow flattens a wrapped ring", ok && ring_empty(&ring));

    /* ---------- insert and remove ---------- */
    ring_clear(&ring);
    for (int i = 0; i < 6; ++i) ring_push_back(&ring, &i);

    int v97 = 97, v98 = 98, v99 = 99;
    ring_insert(&ring, 0, &v97);
    ring_insert(&ring, 4, &v98);
    /* index == size appends */
    ring_insert(&ring, ring_size(&ring), &v99);

    ok = 1;
    {
        int want[] = { 97, 0, 1, 2, 98, 3, 4, 5, 99 };
        for (int i = 0; i < 9; ++i) {
            int *p = ring_get_ptr(&ring, i);
            if (!p || *p != want[i]) ok = 0;
        }
    }
    test_cond("Insert at the front, middle and back", ok && ring_size(&ring) == 9);

    ring_remove(&ring, 4, &out);
    test_cond("Remove the middle object", out == 98 && ring_size(&ring) == 8);

    ring_remove(&ring, 0, &out);
    test_cond("Remove the front object", out == 97 && ring_size(&ring) == 7);

    ring_remove(&ring, ring_size(&ring) - 1, &out);
    test_cond("Remove the back object", out == 99 && ring_size(&ring) == 6);

    ok = 1;
    for (int i = 0; i < 6; ++i) {
        int *p = ring_get_ptr(&ring, i);
        if (!p || *p != i) ok = 0;
    }
    test_cond("Order is kept after insert and remove", ok);

    /* ---------- insert and remove on a wrapped ring ---------- */
    /* a fresh ring, so the slots really wrap: 4 -> 8 -> 16 */
    Ring wr;
    ring_init(&wr, sizeof(int));

    for (int i = 0; i < 16; ++i) ring_push_back(&wr, &i);
    /* head = 10, size = 6: 10 .. 15 */
    for (int i = 0; i < 10; ++i) ring_pop_front(&wr, NULL);
    /* size = 8: 10 .. 15, then 16 and 17 wrap to slot 0 and 1 */
    for (int i = 16; i < 18; ++i) ring_push_back(&wr, &i);

    int v50 = 50;
    ring_insert(&wr, 4, &v50);

    ok = 1;
    {
        int want[] = { 10, 11, 12, 13, 50, 14, 15, 16, 17 };
        for (int i = 0; i < 9; ++i) {
            int *p = ring_get_ptr(&wr, i);
            if (!p || *p != want[i]) ok = 0;
        }
    }
    test_cond("Insert into a wrapped ring", ok && ring_size(&wr) == 9 && wr.head == 0);

    ring_remove(&wr, 4, &out);
    ok = 1;
    for (int i = 0; i < 8; ++i) {
        int *p = ring_get_ptr(&wr, i);
        if (!p || *p != 10 + i) ok = 0;
    }
    test_cond("Remove from a wrapped ring", out == 50 && ok && ring_size(&wr) == 8);

    /* ---------- a wrapped ring whose front piece is the smaller one ---------- */
    /* a naive pair of memmoves corrupts exactly this layout */
    ring_fini(&wr);
    ring_init(&wr, sizeof(int));

    for (int i = 0; i < 16; ++i) ring_push_back(&wr, &i);
    /* head = 12, size = 4: 12 .. 15 */
    for (int i = 0; i < 12; ++i) ring_pop_front(&wr, NULL);
    /* size = 10: 12 .. 15, then 100 .. 105 wrap to slot 0 .. 5 */
    for (int i = 0; i < 6; ++i) {
        int v = 100 + i;
        ring_push_back(&wr, &v);
    }

    int v9 = 9;
    ring_insert(&wr, 0, &v9);

    ok = 1;
    {
        int *p = ring_get_ptr(&wr, 0);
        if (!p || *p != 9) ok = 0;
        for (int i = 0; i < 4; ++i) {
            p = ring_get_ptr(&wr, 1 + i);
            if (!p || *p != 12 + i) ok = 0;
        }
        for (int i = 0; i < 6; ++i) {
            p = ring_get_ptr(&wr, 5 + i);
            if (!p || *p != 100 + i) ok = 0;
        }
    }
    test_cond("Insert flattens a ring whose front piece is smaller", ok && ring_size(&wr) == 11);

    ring_remove(&wr, 0, &out);
    ok = 1;
    for (int i = 0; i < 4; ++i) {
        int *p = ring_get_ptr(&wr, i);
        if (!p || *p != 12 + i) ok = 0;
    }
    for (int i = 0; i < 6; ++i) {
        int *p = ring_get_ptr(&wr, 4 + i);
        if (!p || *p != 100 + i) ok = 0;
    }
    test_cond("Remove from a ring whose front piece is smaller",
              out == 9 && ok && ring_size(&wr) == 10);

    ring_fini(&wr);

    /* ---------- insert into a full ring ---------- */
    ring_clear(&ring);
    int full = ring_capacity(&ring);
    for (int i = 0; i < full; ++i) ring_push_back(&ring, &i);

    int v42 = 42;
    ring_insert(&ring, full / 2, &v42);
    test_cond("Insert into a full ring grows it",
              ring_size(&ring) == full + 1 && *(int *)ring_get_ptr(&ring, full / 2) == 42 &&
                  *(int *)ring_get_ptr(&ring, full) == full - 1);

    /* ---------- clear and reuse ---------- */
    ring_clear(&ring);
    test_cond("Clear leaves the ring empty", ring_empty(&ring) && ring_peek_front(&ring) == NULL);

    for (int i = 0; i < 3; ++i) ring_push_back(&ring, &i);
    ok = 1;
    for (int i = 0; i < 3; ++i) {
        int v = -1;
        ring_pop_front(&ring, &v);
        if (v != i) ok = 0;
    }
    test_cond("Reuse the ring after clear", ok && ring_empty(&ring));

    /* ---------- reserve and the full boundary ---------- */
    ring_clear(&ring);
    ring_reserve(&ring, 1000);
    test_cond("Reserve grows capacity, not size",
              ring_capacity(&ring) >= 1000 && ring_size(&ring) == 0);

    int capacity = ring_capacity(&ring);
    for (int i = 0; i < capacity; ++i) ring_push_back(&ring, &i);
    test_cond("The ring can be completely full", ring_size(&ring) == capacity);

    ok = 1;
    for (int i = 0; i < capacity; ++i) {
        int v = -1;
        ring_pop_front(&ring, &v);
        if (v != i) ok = 0;
    }
    test_cond("A full ring drains in order", ok && ring_empty(&ring));

    /* ---------- out of range ---------- */
    test_cond(
        "insert rejects an out of range index",
        ring_insert(&ring, -1, &v42) == -1 && ring_insert(&ring, ring_size(&ring) + 1, &v42) == -1);
    test_cond(
        "remove rejects an out of range index",
        ring_remove(&ring, -1, &out) == -1 && ring_remove(&ring, ring_size(&ring), &out) == -1);

    /* ---------- NULL tolerance ---------- */
    Ring *nil = NULL;
    test_cond("A NULL ring is empty", ring_empty(nil) && ring_size(nil) == 0);
    test_cond("Operations on a NULL ring are safe",
              ring_get_ptr(nil, 0) == NULL && ring_pop_front(nil, NULL) == -1);

    /* ---------- fini ---------- */
    ring_fini(&ring);
    test_cond("Fini releases the buffer", ring.objs == NULL && ring_capacity(&ring) == 0);

    test_report();
    return 0;
}

int main(void) { return ringTest(); }
