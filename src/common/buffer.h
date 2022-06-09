/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

/* A dynamic-sized buffer. */

#ifndef _KOALA_BUFFER_H_
#define _KOALA_BUFFER_H_

#include "common.h"
#include "mm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
typedef struct _Buffer Buffer;

struct _Buffer {
    /* allocated size */
    int size;
    /* used size */
    int len;
    /* contains string */
    char *buf;
};

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

/* Write 'num' NULL-terminated strings. */
void buf_nwrite(Buffer *self, int num, ...);

/* Write a char into buffer. */
void buf_write_char(Buffer *self, char ch);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BUFFER_H_ */
