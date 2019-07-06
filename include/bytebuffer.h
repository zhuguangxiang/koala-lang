/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _KOALA_BYTEBUFFER_H_
#define _KOALA_BYTEBUFFER_H_

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

/* min size per byteblock */
#define BLOCK_MIN_SIZE 1024

/* max size per byteblock */
#define BLOCK_MAX_SIZE (8 * BLOCK_MIN_SIZE)

/* a dynamic byte buffer */
struct bytebuffer {
  /* block size */
  int bsize;
  /* total bytes in the buffer */
  int total;
  /* number of byteblocks in list */
  int blocks;
  /* byteblock's list */
  struct list_head list;
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

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_BYTEBUFFER_H_ */
