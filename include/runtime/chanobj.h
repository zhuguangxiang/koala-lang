/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_CHAN_OBJECT_H_
#define _KOALA_CHAN_OBJECT_H_

#include "ring.h"
#include "kosched.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A FIFO channel. cap > 0 buffers up to cap values in a ring; cap == 0 is a
 * rendezvous: send hands the value straight to a waiting receiver. Wait
 * queues are FIFO so waiting is fair. close() wakes all receivers with nil
 * and all senders with an error.
 */
typedef struct _ChannelObject {
    OBJECT_HEAD
    /* buffered values, TValue slots */
    Ring buf;
    int cap;
    int closed;
    /* WaitRecord.link; a queued sender's value sits in w->value */
    List sendq;
    List recvq;
} ChannelObject;

extern TypeObject channel_type;
#define IS_CHANNEL(ob) IS_TYPE((ob), &channel_type)

/* the Iterator[T] returned by Channel.iter */
extern TypeObject channel_iter_type;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CHAN_OBJECT_H_ */
