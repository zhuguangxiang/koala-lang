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

#include "buffer.h"
#include "log.h"
#include "mem.h"

LOGGER(1)

Block *block_new(int bsize)
{
  Block *block = Malloc(sizeof(Block) + sizeof(uint8) * bsize);
  assert(block);
  block->used = 0;
  init_list_head(&block->link);
  return block;
}

void block_free(Block *block)
{
  Mfree(block);
}

int Buffer_Init(Buffer *buf, int bsize)
{
  buf->size = 0;
  buf->blocks = 0;
  buf->bsize = bsize;
  init_list_head(&buf->head);
  return 0;
}

int Buffer_Fini(Buffer *buf)
{
  Block *block;
  struct list_head *next = list_first(&buf->head);
  while (next) {
    list_del(next);
    block = container_of(next, Block, link);
    block_free(block);
    next = list_first(&buf->head);
  }
  Buffer_Init(buf, 0);
  return 0;
}

int Buffer_Write(Buffer *buf, uint8 *data, int size)
{
  int left = 0;
  Block *block;
  struct list_head *last;
  int sz;
  while (size > 0) {
    last = list_last(&buf->head);
    if (last) {
      block = container_of(last, Block, link);
      left = buf->bsize - block->used;
      assert(left >= 0);
      Log_Debug("left of block-%d is %d", buf->blocks, left);
    } else {
      Log_Debug("buffer is empty");
      left = 0;
    }

    if (left <= 0) {
      block = block_new(buf->bsize);
      list_add_tail(&block->link, &buf->head);
      buf->blocks++;
      Log_Debug("new block-%d", buf->blocks);
    } else {
      sz = min(left, size);
      memcpy(block->data + block->used, data, sz);
      block->used += sz;
      data += sz;
      size -= sz;
      assert(size >= 0);
      buf->size += sz;
      Log_Debug("write %d-bytes into block-%d", sz, buf->blocks);
    }
  }
  return 0;
}

/* NOTES: caller needs free memory */
uint8 *Buffer_RawData(Buffer *buf)
{
  int size = 0;
  Block *block;
  uint8 *data = Malloc(ALIGN(buf->size, 4));
  assert(data);

  struct list_head *pos;
  list_for_each(pos, &buf->head) {
    block = container_of(pos, Block, link);
    memcpy(data + size, block->data, block->used);
    size += block->used;
  }

  return data;
}
