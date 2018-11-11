/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _KOALA_BUFFER_H_
#define _KOALA_BUFFER_H_

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

/* block structure */
typedef struct block {
  /* linked into Buffer */
  struct list_head link;
  /* this block used bytes */
  int used;
  /* data */
  uint8 data[0];
} Block;

/*
 * buffer structure, not free old memory
 * A buffer consists of N blocks
 * this structure is used as readonly structure, e.g. string buffer
 */
typedef struct buffer {
   /* total bytes in buffer */
  uint32 size;
  /* number of blocks */
  uint32 blocks;
  /* Block size */
  uint32 bsize;
  /* block list */
  struct list_head head;
} Buffer;

/* initialize the buffer */
int Buffer_Init(Buffer *buf, int bsize);
/* fini the buffer, free all blocks in it */
int Buffer_Fini(Buffer *buf);
/* append datas into the buffer */
int Buffer_Write(Buffer *buf, uint8 *data, int sz);
/* to array, need free return memory, must be used mm_free function */
uint8 *Buffer_RawData(Buffer *buf);
/* get the number of stored bytes in the buffer */
#define Buffer_Size(buf) ((buf)->size)
/* write one byte */
#define Buffer_Write_Byte(buf, data) Buffer_Write(buf, &data, 1)
/* write two bytes */
#define Buffer_Write_2Bytes(buf, data) Buffer_Write(buf, (uint8 *)&data, 2)
/* write four bytes */
#define Buffer_Write_4Bytes(buf, data) Buffer_Write(buf, (uint8 *)&data, 4)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_BUFFER_H_ */
