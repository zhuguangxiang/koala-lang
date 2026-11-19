/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "chanobj.h"
#include "excobj.h"
#include "gc.h"

#ifdef __cplusplus
extern "C" {
#endif

static Object *channel_alloc(TypeObject *tp)
{
    ChannelObject *ch = gc_alloc(sizeof(ChannelObject));
    INIT_OBJECT_HEAD(ch, tp, 0);
    ring_init(&ch->buf, sizeof(TValue));
    init_list(&ch->sendq);
    init_list(&ch->recvq);
    return (Object *)ch;
}

/* __init__(cap int = 0) */
static TValue _channel_init(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    ChannelObject *ch = SELF_AS(channel_type);
    int64_t cap = kl_arg_int64(0);
    if (cap < 0) {
        raise_exc_str("channel capacity must be >= 0");
        return error_value;
    }
    ch->cap = (int)cap;
    if (cap > 0) ring_reserve(&ch->buf, (int)cap);
    return no_value;
}

/* hand a value to the first waiting receiver, if any */
static int channel_deliver(ChannelObject *ch, TValue *v)
{
    List *link = list_pop_front(&ch->recvq);
    if (!link) return 0;
    WaitRecord *w = list_entry(link, WaitRecord, link);
    w->value = *v;
    w->result = KL_PARK_OK;
    kl_ready(w->ks);
    return 1;
}

/* send(v T) — parks while full; error if closed. @parks */
static TValue _channel_send(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    ChannelObject *ch = SELF_AS(channel_type);
    if (ch->closed) {
        raise_exc_str("send on closed channel");
        return error_value;
    }
    TValue v = args[0];
    if (channel_deliver(ch, &v)) return no_value;
    if (ch->cap > 0 && ring_size(&ch->buf) < ch->cap) {
        ring_push_back(&ch->buf, &v);
        return no_value;
    }

    WaitRecord w;
    kl_wait_init(&w, WAIT_CHAN_SEND);
    w.value = v;
    list_push_back(&ch->sendq, &w.link);
    int r = kl_park(&w);
    if (r == KL_PARK_CANCELLED) {
        if (!list_empty(&w.link)) list_remove(&w.link);
        raise_exc_str("Cancelled");
        return error_value;
    }
    if (r == KL_PARK_CLOSED) {
        raise_exc_str("send on closed channel");
        return error_value;
    }
    return no_value;
}

/* recv() T? — parks while empty; nil when closed and drained. @parks */
static TValue channel_recv(ChannelObject *ch)
{
    if (ring_size(&ch->buf) > 0) {
        TValue v;
        ring_pop_front(&ch->buf, &v);
        /* refill the buffer from a parked sender */
        List *link = list_pop_front(&ch->sendq);
        if (link) {
            WaitRecord *w = list_entry(link, WaitRecord, link);
            ring_push_back(&ch->buf, &w->value);
            w->result = KL_PARK_OK;
            kl_ready(w->ks);
        }
        return v;
    }

    /* rendezvous: take the value straight from a parked sender */
    List *link = list_pop_front(&ch->sendq);
    if (link) {
        WaitRecord *w = list_entry(link, WaitRecord, link);
        TValue v = w->value;
        w->result = KL_PARK_OK;
        kl_ready(w->ks);
        return v;
    }

    if (ch->closed) return nil_value;

    WaitRecord w;
    kl_wait_init(&w, WAIT_CHAN_RECV);
    list_push_back(&ch->recvq, &w.link);
    int r = kl_park(&w);
    if (r == KL_PARK_CANCELLED) {
        if (!list_empty(&w.link)) list_remove(&w.link);
        raise_exc_str("Cancelled");
        return error_value;
    }
    return w.value;
}

static TValue _channel_recv(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    ChannelObject *ch = SELF_AS(channel_type);
    return channel_recv(ch);
}

/* take a buffered or rendezvous value without parking, nil when none */
static TValue channel_take(ChannelObject *ch)
{
    if (ring_size(&ch->buf) > 0) {
        TValue v;
        ring_pop_front(&ch->buf, &v);
        List *link = list_pop_front(&ch->sendq);
        if (link) {
            WaitRecord *w = list_entry(link, WaitRecord, link);
            ring_push_back(&ch->buf, &w->value);
            w->result = KL_PARK_OK;
            kl_ready(w->ks);
        }
        return v;
    }
    List *link = list_pop_front(&ch->sendq);
    if (link) {
        WaitRecord *w = list_entry(link, WaitRecord, link);
        TValue v = w->value;
        w->result = KL_PARK_OK;
        kl_ready(w->ks);
        return v;
    }
    return nil_value;
}

/* try_send(v T) bool — never parks */
static TValue _channel_try_send(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    ChannelObject *ch = SELF_AS(channel_type);
    if (ch->closed) return bool_value(0);
    TValue v = args[0];
    if (channel_deliver(ch, &v)) return bool_value(1);
    if (ch->cap > 0 && ring_size(&ch->buf) < ch->cap) {
        ring_push_back(&ch->buf, &v);
        return bool_value(1);
    }
    return bool_value(0);
}

/* try_recv() T? — never parks; nil when empty or closed and drained */
static TValue _channel_try_recv(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    ChannelObject *ch = SELF_AS(channel_type);
    return channel_take(ch);
}

/* close() — wake all receivers with nil, all senders with an error */
static TValue _channel_close(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    ChannelObject *ch = SELF_AS(channel_type);
    if (ch->closed) return no_value;
    ch->closed = 1;

    List *link;
    while ((link = list_pop_front(&ch->recvq))) {
        WaitRecord *w = list_entry(link, WaitRecord, link);
        w->value = nil_value;
        w->result = KL_PARK_OK;
        kl_ready(w->ks);
    }
    while ((link = list_pop_front(&ch->sendq))) {
        WaitRecord *w = list_entry(link, WaitRecord, link);
        w->result = KL_PARK_CLOSED;
        kl_ready(w->ks);
    }
    return no_value;
}

/*---------------------------------------------------------------------------+
 |   ChannelIter: the Iterator[T] of a channel, for `for v in ch`            |
 +---------------------------------------------------------------------------*/

typedef struct _ChannelIterObject {
    OBJECT_HEAD
    ChannelObject *ch;
    /* one-value lookahead for has_next */
    TValue peek;
    uint8_t has_peek;
    uint8_t exhausted;
} ChannelIterObject;

TypeObject channel_iter_type;

static Object *channel_iter_alloc(TypeObject *tp)
{
    ChannelIterObject *it = gc_alloc(sizeof(ChannelIterObject));
    INIT_OBJECT_HEAD(it, tp, 0);
    it->peek = nil_value;
    return (Object *)it;
}

/* has_next() bool — parks until a value arrives or the channel closes */
static TValue _channel_iter_has_next(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    ChannelIterObject *it = (ChannelIterObject *)to_obj(self);
    if (it->has_peek) return bool_value(1);
    if (it->exhausted) return bool_value(0);
    TValue v = channel_recv(it->ch);
    if (is_error(&v)) return error_value;
    if (is_nil(&v)) {
        it->exhausted = 1;
        return bool_value(0);
    }
    it->peek = v;
    it->has_peek = 1;
    return bool_value(1);
}

/* next() T? — nil when closed and drained */
static TValue _channel_iter_next(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    ChannelIterObject *it = (ChannelIterObject *)to_obj(self);
    if (it->has_peek) {
        it->has_peek = 0;
        TValue v = it->peek;
        it->peek = nil_value;
        return v;
    }
    if (it->exhausted) return nil_value;
    TValue v = channel_recv(it->ch);
    if (is_error(&v)) return error_value;
    if (is_nil(&v)) it->exhausted = 1;
    return v;
}

/* next_strict() T — panic when exhausted */
static TValue _channel_iter_next_strict(TValue *self, TValue *args, int nargs)
{
    TValue v = _channel_iter_next(self, args, nargs);
    if (is_error(&v)) return error_value;
    if (is_nil(&v)) {
        raise_exc_str("No more elements in ChannelIter");
        return error_value;
    }
    return v;
}

static TValue _channel_iter_init(TValue *self, TValue *args, int nargs);

static MethodDef channel_iter_methods[] = {
    { "__init__", _channel_iter_init },
    { "has_next", _channel_iter_has_next },
    { "next_strict", _channel_iter_next_strict },
    { "next", _channel_iter_next },
    { NULL },
};

static void channel_iter_mark(Object *obj)
{
    ChannelIterObject *it = (ChannelIterObject *)obj;
    gc_mark(it->ch);
    gc_mark_value(&it->peek);
}

TypeObject channel_iter_type = {
    ._type = &type_type,
    .name = "ChannelIter",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(ChannelIterObject),
    ._alloc = channel_iter_alloc,
    ._mark = channel_iter_mark,
    .methdefs = channel_iter_methods,
};

/* ChannelIter[T].__init__(src Channel[T], step int) */
static TValue _channel_iter_init(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    ChannelIterObject *it = (ChannelIterObject *)to_obj(self);
    ASSERT(OB_TYPE((Object *)it) == &channel_iter_type);
    int64_t step = kl_arg_int64(1);
    if (step != 1) {
        raise_exc_str("channel iterator supports step 1 only");
        return error_value;
    }
    Object *src = to_obj(&args[0]);
    ASSERT(IS_CHANNEL(src));
    it->ch = (ChannelObject *)src;
    return no_value;
}

static MethodDef channel_methods[] = {
    { "__init__", _channel_init }, { "send", _channel_send },
    { "recv", _channel_recv },     { "try_send", _channel_try_send },
    { "try_recv", _channel_try_recv }, { "close", _channel_close },
    { NULL },
};

static void channel_mark(Object *obj)
{
    ChannelObject *ch = (ChannelObject *)obj;
    int size = ring_size(&ch->buf);
    for (int i = 0; i < size; i++) gc_mark_value(ring_get_ptr(&ch->buf, i));
    WaitRecord *w;
    list_foreach(w, link, &ch->sendq) gc_mark_value(&w->value);
}

static void channel_free(Object *obj)
{
    ChannelObject *ch = (ChannelObject *)obj;
    ring_fini(&ch->buf);
}

/* pub class Channel[T] { ... } */
TypeObject channel_type = {
    ._type = &type_type,
    .name = "Channel",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(ChannelObject),
    ._alloc = channel_alloc,
    ._mark = channel_mark,
    ._free = channel_free,
    .methdefs = channel_methods,
};

#ifdef __cplusplus
}
#endif
