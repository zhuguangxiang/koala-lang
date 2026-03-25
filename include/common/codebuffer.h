/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_CODEBUFFER_H_
#define _KOALA_CODEBUFFER_H_

#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *data;
    uint32_t size;
    uint32_t capacity;
} CodeBuffer;

static inline void codebuf_init(CodeBuffer *cb)
{
    cb->capacity = 128;
    cb->size = 0;
    cb->data = (uint8_t *)mm_alloc(cb->capacity);
}

static inline void codebuf_fini(CodeBuffer *cb)
{
    mm_free(cb->data);
    cb->data = NULL;
    cb->size = cb->capacity = 0;
}

static inline void codebuf_reserve(CodeBuffer *cb, uint32_t more)
{
    uint32_t need = cb->size + more;
    if (need <= cb->capacity) return;

    while (cb->capacity < need) cb->capacity *= 2;

    cb->data = (uint8_t *)mm_realloc(cb->data, cb->capacity);
}

static inline void codebuf_put_u8(CodeBuffer *cb, uint8_t v)
{
    codebuf_reserve(cb, 1);
    cb->data[cb->size++] = v;
}

static inline void codebuf_put_u16(CodeBuffer *cb, uint16_t v)
{
    codebuf_reserve(cb, 2);
    cb->data[cb->size++] = (v >> 0) & 0xFF;
    cb->data[cb->size++] = (v >> 8) & 0xFF;
}

static inline void codebuf_put_u32(CodeBuffer *cb, uint32_t v)
{
    codebuf_reserve(cb, 4);
    cb->data[cb->size++] = (v >> 0) & 0xFF;
    cb->data[cb->size++] = (v >> 8) & 0xFF;
    cb->data[cb->size++] = (v >> 16) & 0xFF;
    cb->data[cb->size++] = (v >> 24) & 0xFF;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODEBUFFER_H_ */
