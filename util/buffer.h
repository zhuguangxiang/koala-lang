/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

/* A Dynamic Buffer. */

#ifndef _KOALA_BUFFER_H_
#define _KOALA_BUFFER_H_

#include "common.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* buffer */
typedef struct _Buffer {
    /* allocated size */
    int size;
    /* used size */
    int len;
    /* contains string */
    char *buf;
} Buffer, *BufferRef;

/* Declare a buffer. */
#define BUF(name) Buffer name = { 0, 0, NULL }

/* Free a buffer. */
#define FINI_BUF(name) mm_free((name).buf)

// clang-format off
/* Reset a buffer. */
#define RESET_BUF(name) do { \
    memset((name).buf, 0, (name).len); (name).len = 0; \
} while (0)
// clang-format on

/* Get null-terminated string from the buffer. */
#define BUF_STR(name) (name).buf

/* Get the buffer length */
#define BUF_LEN(name) (name).len

/* Write string with len */
void buf_write_nstr(BufferRef self, char *s, int len);

/* Write a null-terminated string. */
void buf_write_str(BufferRef self, char *s);

/* Write 'num' null-terminated strings. */
void buf_nwrite(BufferRef self, int num, ...);
void buf_vwrite(BufferRef self, int num, va_list args);

void buf_write_char(BufferRef self, char ch);
void buf_write_byte(BufferRef self, int val);
void buf_write_int(BufferRef self, int ch);
void buf_write_int64(BufferRef self, int64_t val);
void buf_write_double(BufferRef self, double val);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BUFFER_H_ */
