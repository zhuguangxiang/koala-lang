/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_MEM_H_
#define _KOALA_MEM_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The memory is set to zero. */
void *mm_alloc(int size);

/* The faster memory allocator, not set zero. */
void *mm_alloc_fast(int size);

/* Allocate object by its pointer. */
#define mm_alloc_obj(ptr) mm_alloc(OBJ_SIZE(ptr))

/* Allocate object faster by its pointer, not set zero. */
#define mm_alloc_obj_fast(ptr) mm_alloc_fast(OBJ_SIZE(ptr))

/* realloc */
void *mm_realloc(void *ptr, int new_size);

/* The func frees the memory space pointed by pointer. */
void mm_free(void *ptr);

/* Stat usage memory */
void mm_stat(void);

/* duplicate c-string, replace of strndup. */
static inline char *str_ndup(char *s, size_t size)
{
    char *str = mm_alloc(size + 1);
    memcpy(str, s, size);
    return str;
}

/* duplicate c-string, with extra string. */
static inline char *str_ndup_ex(char *s, size_t size, char *extra)
{
    int msize = size + strlen(extra);
    char *str = mm_alloc(msize + 1);
    memcpy(str, s, size);
    strcat(str, extra);
    return str;
}

/* duplicate c-string, replace of strdup. */
static inline char *str_dup(char *s) { return str_ndup(s, strlen(s)); }

/* duplicate c-string, with extra string. */
static inline char *str_dup_ex(char *s, char *extra)
{
    int size = strlen(s) + strlen(extra);
    char *str = mm_alloc(size + 1);
    strcpy(str, s);
    strcat(str, extra);
    return str;
}

/* trim c-string */
char *str_ntrim(char *s, int len);
char *str_trim(char *s);

/* find ch in reverse order */
int mem_nrchr(char *s, int len, char ch);

/* string split */
int str_sep(char **str, char ch, char **out);

/* string equal */
static inline int str_equal(const char *s1, const char *s2)
{
    // if (!s1 || !s2) return 0;
    if (s1 == s2) return 1;
    return !strcmp(s1, s2);
}

/* string to int64 */
int64_t str_to_int(const char *buf, size_t len, int *ok);

/* string to float64 */
double str_to_float(const char *buf, size_t len, int *ok);

static inline bool match_suffix(const char *name, const char *suffix)
{
    size_t name_len = strlen(name);
    size_t suf_len = strlen(suffix);

    // name is shorter, return false directly
    if (name_len < suf_len) return false;

    return !memcmp(name + (name_len - suf_len), suffix, suf_len);
}

static inline bool match_prefix(const char *name, const char *prefix)
{
    size_t pre_len = strlen(prefix);

    // name is shorter, return false directly
    if (strlen(name) < pre_len) return false;

    return !memcmp(name, prefix, pre_len);
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MEM_H_ */
