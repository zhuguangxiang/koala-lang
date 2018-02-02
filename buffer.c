
#include "buffer.h"
#include "log.h"

Block *block_new(int bsize)
{
  Block *block = malloc(sizeof(Block) + sizeof(uint8) * bsize);
  ASSERT_PTR(block);
  block->used = 0;
  init_list_head(&block->link);
  return block;
}

void block_free(Block *block)
{
  free(block);
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
  while (next != NULL) {
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
    if (last != NULL) {
      block = container_of(last, Block, link);
      left = buf->bsize - block->used;
      ASSERT(left >= 0);
      info("left of block-%d is %d\n", buf->blocks, left);
    } else {
      info("buffer is empty\n");
      left = 0;
    }

    if (left <= 0) {
      block = block_new(buf->bsize);
      list_add_tail(&block->link, &buf->head);
      buf->blocks++;
      info("new block-%d\n", buf->blocks);
    } else {
      sz = min(left, size);
      memcpy(block->data + block->used, data, sz);
      block->used += sz;
      data += sz;
      size -= sz;
      ASSERT(size >= 0);
      buf->size += sz;
      info("write %d-bytes into block-%d\n", sz, buf->blocks);
    }
  }
  return 0;
}

uint8 *Buffer_RawData(Buffer *buf)
{
  int size = 0;
  Block *block;
  uint8 *data = malloc(ALIGN_UP(buf->size, 4));
  ASSERT_PTR(data);
  list_for_each_entry(block, &buf->head, link) {
    memcpy(data + size, block->data, block->used);
    size += block->used;
  }
  return data;
}
