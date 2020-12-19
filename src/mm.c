/*===-- mm.c - Common Memory Allocator ----------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file implements common mmeory allocator.                              *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The head of user's memory */
typedef struct Block {
    /* allocated size */
    size_t size;
    /* memory guard */
    uint32_t magic;
} Block;

/* allocated memory size */
static int usedsize = 0;

/* the heap size */
static const int maxsize = 512 * 1024;

void *mm_alloc(int size)
{
    if (usedsize >= maxsize) {
        printf("error: there is no more memory for heap allocator.\n");
        abort();
    }

    Block *blk = calloc(1, sizeof(*blk) + size);
    assert(blk);
    blk->size = size;
    blk->magic = 0xdeadbeaf;
    usedsize += size;

    return (void *)(blk + 1);
}

void mm_free(void *ptr)
{
    if (!ptr) return;

    Block *blk = (Block *)ptr - 1;

    if (blk->magic != 0xdeadbeaf) {
        printf("bug: memory is broken.\n");
        abort();
    }

    usedsize -= blk->size;
    free(blk);
}

void mm_stat(void)
{
    puts("------ Memory Usage ------");
    printf("%d bytes available\n", maxsize - usedsize);
    printf("%d bytes used\n", usedsize);
    puts("--------------------------");
}

#ifdef __cplusplus
}
#endif
