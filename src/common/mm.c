/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "common.h"
#include "log.h"

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

/* memory is cleared to zero. */
void *mm_alloc(int size)
{
    Block *blk = calloc(1, OBJ_SIZE(blk) + size);
    if (!blk) {
        log_fatal("calloc failed.");
        abort();
    }

    blk->size = size;
    blk->magic = GUARD_MAGIC;
    used_size += size;

    return (void *)(blk + 1);
}

/* memory is not cleared. */
void *mm_alloc_fast(int size)
{
    Block *blk = malloc(OBJ_SIZE(blk) + size);
    if (!blk) {
        log_fatal("malloc failed.");
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
        log_fatal("memory is broken.\n");
        abort();
    }

    used_size -= blk->size;
    free(blk);
}

void mm_stat(void)
{
    puts("------ Memory Usage ------");
    printf("%d bytes used\n", used_size);
    puts("--------------------------");
}

#ifdef __cplusplus
}
#endif
