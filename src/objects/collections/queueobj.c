/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "collections/queueobj.h"
#include "excobj.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Elements are stored by value (TValue); the ring's buffer holds them, and
 * `ring.size` is the number of live elements. */
#define QUEUE_ELEM_SIZE ((int)sizeof(TValue))

/* Queues longer than QUEUE_FMT_MAX only show QUEUE_FMT_EDGE items at each end. */
#define QUEUE_FMT_MAX  10
#define QUEUE_FMT_EDGE 4

/* Number of live elements. */
static inline int _queue_size(QueueObject *q) { return ring_size(&q->ring); }

/* Pointer to the live element at `index`, 0 <= index < _queue_size(). */
static inline TValue *_queue_at(QueueObject *q, int index)
{
    return (TValue *)ring_get_ptr(&q->ring, index);
}

// __init__()
static TValue _queue_init(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    QueueObject *q = SELF_AS(queue_type);
    ring_init(&q->ring, QUEUE_ELEM_SIZE);
    return no_value;
}

// len() -> int
static TValue _queue_len(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    QueueObject *q = SELF_AS(queue_type);
    return int64_value(_queue_size(q));
}

// empty() -> bool
static TValue _queue_empty(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    QueueObject *q = SELF_AS(queue_type);
    return bool_value(ring_empty(&q->ring));
}

// push(v T)
static TValue _queue_push(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    QueueObject *q = SELF_AS(queue_type);
    /* `mm_alloc` aborts when out of memory, so the push cannot fail. */
    ring_push_back(&q->ring, &args[0]);
    return no_value;
}

// pop() -> T
static TValue _queue_pop(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    QueueObject *q = SELF_AS(queue_type);

    TValue value = nil_value;
    if (ring_pop_front(&q->ring, &value)) {
        raise_exc_str("pop from empty queue");
        return error_value;
    }

    return value;
}

// peek() -> T
static TValue _queue_peek(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    QueueObject *q = SELF_AS(queue_type);

    TValue *front = ring_peek_front(&q->ring);
    if (!front) {
        raise_exc_str("peek from empty queue");
        return error_value;
    }

    return *front;
}

// clear()
static TValue _queue_clear(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    QueueObject *q = SELF_AS(queue_type);
    /* Keep the buffer, drop the elements. */
    ring_clear(&q->ring);
    return no_value;
}

// fmt(f Formatter)
static TValue _queue_fmt(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    QueueObject *q = SELF_AS(queue_type);
    Formatter *fmt = kl_arg_fmt(0);

    int size = _queue_size(q);

    buf_write_str(&fmt->buf, "Queue([");

    if (size <= QUEUE_FMT_MAX) {
        for (int i = 0; i < size; ++i) {
            if (i > 0) buf_write_str(&fmt->buf, ", ");
            kl_call_fmt(_queue_at(q, i), fmt);
        }
    } else {
        /* Show the first QUEUE_FMT_EDGE and the last QUEUE_FMT_EDGE items. */
        for (int i = 0; i < QUEUE_FMT_EDGE; ++i) {
            if (i > 0) buf_write_str(&fmt->buf, ", ");
            kl_call_fmt(_queue_at(q, i), fmt);
        }

        buf_write_str(&fmt->buf, ", ..., ");

        for (int i = size - QUEUE_FMT_EDGE; i < size; ++i) {
            if (i > size - QUEUE_FMT_EDGE) buf_write_str(&fmt->buf, ", ");
            kl_call_fmt(_queue_at(q, i), fmt);
        }
    }

    buf_write_str(&fmt->buf, "])");
    return no_value;
}

static MethodDef queue_methods[] = {
    { "__init__", _queue_init }, { "len", _queue_len }, { "empty", _queue_empty },
    { "push", _queue_push },     { "pop", _queue_pop }, { "peek", _queue_peek },
    { "clear", _queue_clear },   { "fmt", _queue_fmt }, { NULL },
};

/* pub class Queue[T] { ... } */
TypeObject queue_type = {
    ._type = &type_type,
    .name = "Queue",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(QueueObject),
    .methdefs = queue_methods,
};

Object *kl_new_queue(void)
{
    QueueObject *q = mm_alloc_obj(q);
    INIT_OBJECT_HEAD(q, &queue_type, 0);
    ring_init(&q->ring, QUEUE_ELEM_SIZE);
    return (Object *)q;
}

#ifdef __cplusplus
}
#endif
