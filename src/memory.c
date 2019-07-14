/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <stdio.h>
#include <inttypes.h>
#include "memory.h"

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
  assert(blk);
  maxsize += size;
  usedsize += size;
  blk->size = size;
  blk->mask = 0xdeadbeaf;
  return (void *)(blk + 1);
}

void __kfree(void *ptr)
{
  struct block *blk = (struct block *)ptr - 1;
  assert(blk->mask == 0xdeadbeaf);
  usedsize -= blk->size;
  free(blk);
  assert(usedsize >= 0);
}

void kstat(void)
{
  puts("------ Memory Usage ------");
  printf("|  Max: %d Bytes\n|  Current: %d Bytes\n", maxsize, usedsize);
  puts("--------------------------");
}
