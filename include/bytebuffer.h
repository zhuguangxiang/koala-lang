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
struct bytebuffer {
  /* block size */
  int bsize;
  /* total bytes in the buffer */
  int total;
  /* byteblock's vector */
  struct vector vec;
};

/*
 * Initialize the byte buffer.
 *
 * self - The buffer to be initialized.
 * size - The each block size.
 *
 * Returns nothing.
 */
void bytebuffer_init(struct bytebuffer *self, int bsize);

/*
 * Free the byte buffer.
 *
 * self - The buffer to be freed.
 *
 * Returns nothing.
 */
void bytebuffer_free(struct bytebuffer *self);

/*
 * Write n bytes data into byte buffer.
 *
 * self - The buffer to write.
 * data - The data to be written into buffer.
 * size - The data size.
 *
 * Returns 0 successful or -1 if memory allocation failed.
 */
int bytebuffer_write(struct bytebuffer *self, char *data, int size);

/*
 * Get byte buffer size.
 *
 * self - The buffer to get its size.
 *
 * Returns buffer's size.
 */
#define bytebuffer_size(self) (self)->total

/*
 * Write 1 byte data into byte buffer.
 *
 * self - The buffer to write.
 * data - The data to be written into buffer.
 *
 * Returns 0 successful or -1 if memory allocation failed.
 */
#define bytebuffer_write_byte(self, data) \
  bytebuffer_write(self, (char *)&data, 1)

/*
 * Write 2 bytes data into byte buffer.
 *
 * self - The buffer to write.
 * data - The data to be written into buffer.
 *
 * Returns 0 successful or -1 if memory allocation failed.
 */
#define bytebuffer_write_2bytes(self, data) \
  bytebuffer_write(self, (char *)&data, 2)

/*
 * Write 4 bytes data into byte buffer.
 *
 * self - The buffer to write.
 * data - The data to be written into buffer.
 *
 * Returns 0 successful or -1 if memory allocation failed.
 */
#define bytebuffer_write_4bytes(self, data) \
  bytebuffer_write(self, (char *)&data, 4)

/*
 * output bytes into an array.
 *
 * self - The buffer to output.
 * arr  - The array that datas stored in.
 *
 * Returns size of bytes to output.
 */
int bytebuffer_toarr(struct bytebuffer *self, char **arr);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_BYTEBUFFER_H_ */
