/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_QUEUE_OBJECT_H_
#define _KOALA_QUEUE_OBJECT_H_

#include "object.h"
#include "ring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _QueueObject {
    OBJECT_HEAD
    /* Elements are `TValue`, stored by value in a circular buffer: the front
     * element is `ring.head` and `ring.size` is the number of live elements.
     * Both ends are strictly O(1), and draining the queue never has to
     * compact the buffer. */
    Ring ring;
} QueueObject;

extern TypeObject queue_type;
#define IS_QUEUE(ob) IS_TYPE((ob), &queue_type)

Object *kl_new_queue(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_QUEUE_OBJECT_H_ */