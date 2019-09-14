/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#include "bytebuffer.h"
#include "log.h"

struct byteblock {
  int used;
  char data[0];
};

void bytebuffer_init(ByteBuffer *self, int bsize)
{
  self->bsize = bsize;
  self->total = 0;
  vector_init(&self->vec);
}

void bytebuffer_fini(ByteBuffer *self)
{
  void *block;
  vector_for_each(block, &self->vec) {
    kfree(block);
  }
  vector_fini(&self->vec);
  memset(self, 0, sizeof(*self));
}

static inline struct byteblock *alloc_byteblock(int size)
{
  return kmalloc(sizeof(struct byteblock) + size);
}

int bytebuffer_write(ByteBuffer *self, char *data, int size)
{
  int left;
  int min;
  struct byteblock *block;
  while (size > 0) {
    block = vector_top_back(&self->vec);
    left = block ? self->bsize - block->used : 0;
    if (left <= 0) {
      block = alloc_byteblock(self->bsize);
      vector_push_back(&self->vec, block);
    } else {
      min = left > size ? size : left;
      memcpy(block->data + block->used, data, min);
      block->used += min;
      expect(block->used <= self->bsize);
      data += min;
      size -= min;
      expect(size >= 0);
      self->total += min;
    }
  }
  return 0;
}

int bytebuffer_toarr(ByteBuffer *self, char **arr)
{
  char *buf = kmalloc((self->total + 3) & ~3);
  struct byteblock *block;
  int index = 0;
  vector_for_each(block, &self->vec) {
    memcpy(buf + index, block->data, block->used);
    index += block->used;
  }
  *arr = buf;
  return index;
}
