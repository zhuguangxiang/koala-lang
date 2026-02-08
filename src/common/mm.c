/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "common.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The head of user's memory */
typedef struct _Block {
    /* allocated size */
    uint32_t size;
    /* memory guard */
    uint32_t magic;
#define GUARD_MAGIC 0xdeadbeaf
} Block;

typedef union _BlockWrap {
    void *ptr;
    Block blk;
} BlockWrap;

typedef struct _Heap {
    BlockWrap *free_list;
    uint32_t allocated;
    uint32_t cached;
} Heap;

/* 2^5 * (1 -- 16) */
static Heap heaps[16];

/* allocated memory size */
static int used_size = 0;

/* memory is cleared to zero. */
void *mm_alloc(int size)
{
    size = ALIGN(size, 32);

    if (size >= 32) {
        int slot = (size >> 5) - 1;

        if (slot >= 0 && slot <= 15) {
            Heap *hp = &heaps[slot];
            if (hp->free_list) {
                // printf("mm_alloc from free list\n");
                BlockWrap *wrap = hp->free_list;
                hp->free_list = wrap->ptr;
                Block *blk = (Block *)wrap;
                blk->size = size;
                blk->magic = GUARD_MAGIC;
                memset(blk + 1, 0, size);
                --hp->cached;
                return (void *)(blk + 1);
            }
        }
    }

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
    size = ALIGN(size, 32);

    if (size >= 32) {
        int slot = (size >> 5) - 1;

        if (slot >= 0 && slot <= 15) {
            Heap *hp = &heaps[slot];
            if (hp->free_list) {
                // printf("mm_alloc_fast from free list\n");
                BlockWrap *wrap = hp->free_list;
                hp->free_list = wrap->ptr;
                Block *blk = (Block *)wrap;
                blk->size = size;
                blk->magic = GUARD_MAGIC;
                return (void *)(blk + 1);
            }
        }
    }

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
    if (blk->size >= 32) {
        BlockWrap *wrap = (BlockWrap *)blk;
        int slot = (blk->size >> 5) - 1;
        if (slot >= 0 && slot <= 15) {
            Heap *hp = &heaps[slot];
            wrap->ptr = hp->free_list;
            hp->free_list = wrap;
            ++hp->cached;
        }
    } else {
        free(blk);
    }
}

void mm_stat(void)
{
    puts("------ Memory Usage ------");
    printf("%d bytes used\n", used_size);
    puts("--------------------------");
}

char *str_ntrim(char *s, int len)
{
    if (s == NULL) return NULL;
    if (len == 0) return NULL;

    char *fp = s;
    char *ep = s + len - 1;

    while (isspace(*fp)) {
        ++fp;
    }
    if (ep != fp) {
        while (isspace(*ep) && ep != fp) {
            --ep;
        }
    }

    if (ep == fp) return NULL;
    return strndup(fp, ep - fp + 1);
}

char *str_trim(char *s) { return str_ntrim(s, strlen(s)); }

int mem_nrchr(char *s, int len, char ch)
{
    int count = len - 1;
    char *end = s + count;
    while (end != s) {
        if (*end == ch) return count;
        --end;
        --count;
    }
    return -1;
}

int str_sep(char **str, char ch, char **out)
{
    if (*str == NULL) return 0;

    char *s = *str;
    int count = 0;
    while (*s) {
        if (*s == ch) {
            *out = *str;
            *str = s + 1;
            return count;
        }
        s++;
        count++;
    }
    *out = *str;
    *str = NULL;
    return count;
}

#ifdef __cplusplus
}
#endif
