/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <stdio.h>
#include <inttypes.h>
#include "memory.h"
#include "log.h"

/* max allocated memory size */
int maxsize;

/* current allocated memory size */
int usedsize;

struct block {
  size_t size;
  uint32_t mask;
};

void *kmalloc(size_t size)
{
  struct block *blk = calloc(1, sizeof(struct block) + size);
  if (!blk)
    panic("null pointer");
  maxsize += size;
  usedsize += size;
  blk->size = size;
  blk->mask = 0xdeadbeaf;
  return (void *)(blk + 1);
}

void __kfree(void *ptr)
{
  struct block *blk = (struct block *)ptr - 1;
  if (blk->mask != 0xdeadbeaf)
    panic("block magic check failed.");
  usedsize -= blk->size;
  free(blk);
  if (usedsize < 0)
    panic("unexpected error");
}

void kstat(void)
{
  puts("------ Memory Usage ------");
  print("|  Max: %d Bytes\n|  Current: %d Bytes\n", maxsize, usedsize);
  puts("--------------------------");
}
