/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_BYTEBUF_OBJECT_H_
#define _KOALA_BYTEBUF_OBJECT_H_

#include "buffer.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ByteBufObject {
    OBJECT_HEAD
    Buffer buf;
} ByteBufObject;

extern TypeObject ByteBuf_type;
#define IS_BYTEBUF(ob) IS_TYPE((ob), &ByteBuf_type)
Object *kl_new_bytebuf(size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BYTEBUF_OBJECT_H_ */
