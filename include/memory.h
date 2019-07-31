/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_MEMORY_H_
#define _KOALA_MEMORY_H_

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Allocate memory for size. */
void *kmalloc(size_t size);

/* Free memory which is allocated by 'kmalloc'. */
void __kfree(void *ptr);
#define kfree(ptr) \
({                 \
  if (ptr)         \
    __kfree(ptr);  \
  ptr = NULL;      \
})

/* Stat memory usage with 'kmalloc' and 'kfree'. */
void kstat(void);

/* duplicate c-string, replace of strndup. */
static inline char *string_ndup(char *s, size_t size)
{
  char *str = kmalloc(size + 1);
  memcpy(str, s, size);
  return str;
}

/* duplicate c-string, with extra available size. */
static inline char *string_ndup_extra(char *s, size_t size, size_t extra)
{
  char *str = kmalloc(size + extra + 1);
  memcpy(str, s, size);
  return str;
}

/* duplicate c-string, replace of strdup. */
static inline char *string_dup(char *s)
{
  return string_ndup(s, strlen(s));
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MEMORY_H_ */
