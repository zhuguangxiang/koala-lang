/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/* A dynamic-sized buffer. */

#ifndef _KOALA_BUFFER_H_
#define _KOALA_BUFFER_H_

#include "common.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Buffer {
    /* allocated size */
    int size;
    /* used size */
    int len;
    /* contains string */
    char *buf;
} Buffer;

/* Declare a buffer. */
#define BUF(name) Buffer name = { 0, 0, NULL }

/* Free a buffer. */
#define FINI_BUF(name) mm_free((name).buf)

/* Init a buffer */
#define INIT_BUF(name) memset(&(name), 0, sizeof(Buffer))

// clang-format off
/* Reset a buffer. */
#define RESET_BUF(name) memset((name).buf, 0, (name).len); (name).len = 0;
// clang-format on

/* Get NULL-terminated string from the buffer. */
#define BUF_STR(name) (name).buf

/* Get the buffer length */
#define BUF_LEN(name) (name).len

/* Write a NULL-terminated string. */
void buf_write_str(Buffer *self, char *s);

/* Write string with len */
void buf_write_nstr(Buffer *self, char *s, int len);

/* Write 'count' NULL-terminated strings. */
void buf_nwrite(Buffer *self, int count, ...);

/* Write a char into buffer. */
void buf_write_char(Buffer *self, char ch);

/* Write an int64 into buffer. */
void buf_write_int64(Buffer *self, int64_t val);

/* Write a double into buffer. */
void buf_write_double(Buffer *self, double val);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BUFFER_H_ */
