/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <math.h>
#include "common.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The head of user's memory(16 bytes aligned) */
typedef struct _Block {
    /* allocated size */
    uint32_t size;
    /* memory guard */
    uint32_t magic;
#define GUARD_MAGIC 0xdeadbeaf
    uint32_t unused[2];
} Block;

/* allocated memory size */
static int used_size = 0;

/* memory is cleared to zero. */
void *mm_alloc(int size)
{
    size = ALIGN(size, 16);
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

void *mm_realloc(void *ptr, int new_size)
{
    if (!ptr) return mm_alloc(new_size);

    Block *blk = (Block *)ptr - 1;

    if (blk->magic != GUARD_MAGIC) {
        log_fatal("memory is broken.\n");
        abort();
    }

    int old_size = blk->size;
    blk = (Block *)realloc(blk, OBJ_SIZE(blk) + new_size);
    if (!blk) {
        log_fatal("realloc failed.");
        abort();
    }

    blk->size = new_size;
    blk->magic = GUARD_MAGIC;
    used_size += (new_size - old_size);

    return (void *)(blk + 1);
}

/* memory is not cleared. */
void *mm_alloc_fast(int size)
{
    size = ALIGN(size, 16);
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
    if (str == NULL || *str == NULL) return 0;

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

int64_t str_to_int(const char *buf, size_t len, int *ok)
{
    *ok = 0;
    if (len == 0) return 0;

    // must copy to null-terminated buffer
    char tmp[256];
    if (len >= sizeof(tmp)) return 0; // too long
    memcpy(tmp, buf, len);
    tmp[len] = '\0';

    char *endptr = NULL;
    errno = 0;

    int64_t val = strtoll(tmp, &endptr, 10);

    // strict mode: must consume all characters
    if (endptr != tmp + len) return 0;

    // overflow
    if (errno == ERANGE) return 0;

    *ok = 1;
    return val;
}

double str_to_float(const char *buf, size_t len, int *ok)
{
    *ok = 0;
    if (len == 0) return 0.0;

    char tmp[256];
    if (len >= sizeof(tmp)) return 0.0;
    memcpy(tmp, buf, len);
    tmp[len] = '\0';

    char *endptr = NULL;
    errno = 0;

    double val = strtod(tmp, &endptr);

    // strict mode: must consume all characters
    if (endptr != tmp + len) return 0.0;

    // reject inf/nan
    if (!isfinite(val)) return 0.0;

    // overflow
    if (errno == ERANGE) return 0.0;

    *ok = 1;
    return val;
}

#ifdef __cplusplus
}
#endif
