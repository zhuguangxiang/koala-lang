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

#ifndef _KOALA_MEM_H_
#define _KOALA_MEM_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct memstat {
  /* max allocated */
  long long allocated;
  /* per statement used */
  long long used;
} MemStat;

extern MemStat memstat;

/* no gc memory allocator */
#define mm_alloc(size)       \
({                           \
  memstat.allocated += size; \
  memstat.used += size;      \
  calloc(1, size);           \
})

#define mm_free(ptr)              \
({                                \
  if (ptr != NULL) {              \
    free(ptr);                    \
    memstat.used -= sizeof(*ptr); \
  }                               \
})

void show_memstat(void);

/* gc memory allocator */
void *gc_alloc(int size);
void gc_free(void *ptr);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_MEM_H_ */
