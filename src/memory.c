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

#include <assert.h>
#include <string.h>
#include "memory.h"

#define PAGE_SIZE  4096
#define CACHE_SIZE 16

#define SIZED_CACHE_INIT(size) {"cache-"#size, size, 0, NULL, NULL}

struct cache size_caches[CACHE_SIZE] = {
  SIZED_CACHE_INIT(32),  SIZED_CACHE_INIT(64),
  SIZED_CACHE_INIT(96),  SIZED_CACHE_INIT(128),
  SIZED_CACHE_INIT(160), SIZED_CACHE_INIT(192),
  SIZED_CACHE_INIT(224), SIZED_CACHE_INIT(256),
  SIZED_CACHE_INIT(288), SIZED_CACHE_INIT(320),
  SIZED_CACHE_INIT(352), SIZED_CACHE_INIT(384),
  SIZED_CACHE_INIT(416), SIZED_CACHE_INIT(448),
  SIZED_CACHE_INIT(480), SIZED_CACHE_INIT(512)
};

void *kmalloc(size_t size)
{
  void *ptr;

  /* greater than 512 bytes, use malloc */
  if (size > size_caches[CACHE_SIZE - 1].objsze) {
    ptr = malloc(size + sizeof(size_t));
    assert(ptr != NULL);
    *(size_t *)ptr = size;
    return (void *)((size_t *)ptr + 1);
  }

  size = (size + 32) & 0x20;
  int index = size >> 6;
  return kcache_alloc(size_caches + index);
}


void kfree(void *ptr)
{
  size_t *sizep = (size_t *)ptr - 1;
  if (*sizep > 512) {
    free(sizep);
    return;
  }

  int index = ((*sizep + 32) & 0x20) >> 6;
  kcache_restore(size_caches + index, sizep);
}
