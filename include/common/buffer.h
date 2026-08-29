/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/* A dynamic-sized buffer. */

#ifndef _KOALA_BUFFER_H_
#define _KOALA_BUFFER_H_

#include <stddef.h>
#include <stdint.h>

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

/* Reserve space in the buffer. */
int buf_reserve(Buffer *self, size_t size);

/* Write a NULL-terminated string. */
void buf_write_str(Buffer *self, const char *s);

/* Write string with len */
void buf_write_nstr(Buffer *self, const char *s, int len);

/* Write 'count' NULL-terminated strings. */
void buf_nwrite(Buffer *self, int count, ...);

/* Write a char into buffer. */
void buf_write_char(Buffer *self, char ch);

/* Write a byte into buffer. */
void buf_write_byte(Buffer *self, uint8_t val);

/* Write a word into buffer. */
void buf_write_word(Buffer *self, uint16_t val);

/* write an uint32(not str) into buffer */
void buf_write_uint32(Buffer *self, uint32_t val);

/* Write an int64 str(not int64 self) into buffer. */
void buf_write_int64_str(Buffer *self, int64_t val);

/* Write a double str(not double self) into buffer. */
void buf_write_double_str(Buffer *self, double val);

/* Escape a string and write it into the buffer. */
void escape_str(const char *s, Buffer *buf);

/* Write an uint8 as hex into buffer. */
void buf_write_uint8_hex(Buffer *self, uint8_t val);

/* Write a formatted string into the buffer. */
void buf_write_fmt(Buffer *self, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BUFFER_H_ */
