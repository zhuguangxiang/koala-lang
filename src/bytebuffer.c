/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
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

static void byteblock_free_cb(void *block, void *data)
{
  kfree(block);
}

void bytebuffer_fini(ByteBuffer *self)
{
  vector_fini(&self->vec, byteblock_free_cb, NULL);
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
      if (block->used > self->bsize)
        panic("unexpected error");
      data += min;
      size -= min;
      if (size < 0)
        panic("unexpected error");
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
  vector_iterator(iter, &self->vec);
  iter_for_each(&iter, block) {
    memcpy(buf + index, block->data, block->used);
    index += block->used;
  }
  *arr = buf;
  return index;
}
