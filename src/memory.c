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

#include <stdio.h>
#include <inttypes.h>
#include <ctype.h>
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
  expect(blk != NULL);
  maxsize += size;
  usedsize += size;
  blk->size = size;
  blk->mask = 0xdeadbeaf;
  return (void *)(blk + 1);
}

void __kfree(void *ptr)
{
  struct block *blk = (struct block *)ptr - 1;
  expect(blk->mask == 0xdeadbeaf);
  usedsize -= blk->size;
  free(blk);
  expect(usedsize >= 0);
}

void kstat(void)
{
  puts("------ Memory Usage ------");
  print("|  Max: %d Bytes\n|  Current: %d Bytes\n", maxsize, usedsize);
  puts("--------------------------");
}

char *str_ntrim(char *s, int len)
{
  if (s == NULL) return NULL;
  if (len == 0) return NULL;

  char *fp = s;
  char *ep = s + len - 1;

  while (isspace(*fp)) { ++fp; }
  if (ep != fp) {
    while (isspace(*ep) && ep != fp) { --ep; }
  }

  if (ep == fp) return NULL;
  return strndup(fp, ep - fp + 1);
}

char *str_trim(char *s)
{
  return str_ntrim(s, strlen(s));
}
