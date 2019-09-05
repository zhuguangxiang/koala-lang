/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_BYTEBUFFER_H_
#define _KOALA_BYTEBUFFER_H_

#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* a dynamic byte buffer */
typedef struct bytebuffer {
  /* block size */
  int bsize;
  /* total bytes in the buffer */
  int total;
  /* byteblock's vector */
  Vector vec;
} ByteBuffer;

/* Initialize the byte buffer. */
void bytebuffer_init(ByteBuffer *self, int bsize);

/* Free the byte buffer. */
void bytebuffer_fini(ByteBuffer *self);

/* Write n bytes data into byte buffer. */
int bytebuffer_write(ByteBuffer *self, char *data, int size);

/* Get byte buffer size. */
#define bytebuffer_size(self) (self)->total

/* Write one byte data into byte buffer. */
#define bytebuffer_write_byte(self, data) \
  bytebuffer_write(self, (char *)&data, 1)

/* Write 2 bytes data into byte buffer. */
#define bytebuffer_write_2bytes(self, data) \
  bytebuffer_write(self, (char *)&data, 2)

/* Write 4 bytes data into byte buffer. */
#define bytebuffer_write_4bytes(self, data) \
  bytebuffer_write(self, (char *)&data, 4)

/* Convert byte buffer to an array. */
int bytebuffer_toarr(ByteBuffer *self, char **arr);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BYTEBUFFER_H_ */
