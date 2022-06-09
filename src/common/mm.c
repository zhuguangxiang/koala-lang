/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
typedef struct _Block Block;

/* The head of user's memory */
struct _Block {
    /* allocated size */
    uint32_t size;
    /* memory guard */
    uint32_t magic;
#define GUARD_MAGIC 0xdeadbeaf
};

/* allocated memory size */
static int used_size = 0;

void *mm_alloc(int size)
{
    /* MEM_MAXSIZE is defined in cmake */
    if (used_size >= MEM_MAXSIZE) {
        log_error("there is no more memory for heap allocator.");
        abort();
    }

    Block *blk = calloc(1, OBJ_SIZE(blk) + size);
    if (!blk) {
        log_error("calloc failed.");
        abort();
    }

    blk->size = size;
    blk->magic = GUARD_MAGIC;
    used_size += size;

    return (void *)(blk + 1);
}

void mm_free(void *ptr)
{
    if (!ptr) return;

    Block *blk = (Block *)ptr - 1;

    if (blk->magic != GUARD_MAGIC) {
        log_error("memory is broken.\n");
        abort();
    }

    used_size -= blk->size;
    free(blk);
}

void mm_stat(void)
{
    puts("------ Memory Usage ------");
    /* MEM_MAXSIZE is defined in cmake */
    printf("%d bytes available\n", MEM_MAXSIZE - used_size);
    printf("%d bytes used\n", used_size);
    puts("--------------------------");
}

#ifdef __cplusplus
}
#endif
