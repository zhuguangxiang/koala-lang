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

void bytebuffer_init(struct bytebuffer *self, int bsize)
{
  self->bsize = bsize;
  self->total = 0;
  vector_init(&self->vec, sizeof(void *));
}

static void __byteblock_free_cb__(void *block, void *data)
{
  kfree(*(void **)block);
}

void bytebuffer_free(struct bytebuffer *self)
{
  vector_free(&self->vec, __byteblock_free_cb__, NULL);
  memset(self, 0, sizeof(*self));
}

static inline struct byteblock *alloc_byteblock(int size)
{
  struct byteblock *block = kmalloc(sizeof(*block) + size);
  return block;
}

int bytebuffer_write(struct bytebuffer *self, char *data, int size)
{
  int left;
  int min;
  struct byteblock *block;
  while (size > 0) {
    block = NULL;
    vector_top_back(&self->vec, &block);
    left = block ? self->bsize - block->used : 0;
    if (left <= 0) {
      block = alloc_byteblock(self->bsize);
      vector_push_back(&self->vec, &block);
    } else {
      min = left > size ? size : left;
      memcpy(block->data + block->used, data, min);
      block->used += min;
      panic(block->used > self->bsize, "unexpected error");
      data += min;
      size -= min;
      panic(size < 0, "unexpected error");
      self->total += min;
    }
  }
  return 0;
}

int bytebuffer_toarr(struct bytebuffer *self, char **arr)
{
  VECTOR_ITERATOR(iter, &self->vec);
  char *buf = kmalloc((self->total + 3) & ~3);
  struct byteblock *block;
  int index = 0;
  iter_for_each_as(&iter, struct byteblock *, block) {
    memcpy(buf + index, block->data, block->used);
    index += block->used;
  }
  *arr = buf;
  return index;
}
