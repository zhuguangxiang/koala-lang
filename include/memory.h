/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_MEMORY_H_
#define _KOALA_MEMORY_H_

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * allocate memory for size.
 *
 * size - The memory size to be allocated.
 *
 * Returns the memory or null if memory allocation failed.
 */
void *kmalloc(size_t size);

/*
 * free memory which is allocated by 'kmalloc'.
 *
 * ptr - The memory to be freed.
 *
 * Returns nothing.
 */
void __kfree(void *ptr);
#define kfree(ptr) \
({                 \
  if (ptr)         \
    __kfree(ptr);  \
  ptr = NULL;      \
})

/*
 * stat memory usage with 'kmalloc' and 'kfree'.
 *
 * Returns nothing.
 */
void kstat(void);

/*
 * duplicate c-string, replace of strndup.
 *
 * s    - The origin string.
 * size - The size to copy.
 *
 * Returns a duplicated string with '\0'.
 */
static inline char *string_ndup(char *s, size_t size)
{
  char *str = kmalloc(size + 1);
  memcpy(str, s, size);
  return str;
}

/*
 * duplicate c-string, with extra available size.
 *
 * msize - The duplicated string memory size.
 * s     - The origin string.
 * size  - The size to copy.
 *
 * Returns a duplicated string with '\0'.
 */
static inline char *string_nndup(size_t msize, char *s, size_t size)
{
  assert(msize >= size);
  char *str = kmalloc(msize + 1);
  memcpy(str, s, size);
  return str;
}

/*
 * duplicate c-string, replace of strdup
 *
 * s    - The origin string.
 *
 * Returns a duplicated string with '\0'.
 */
static inline char *string_dup(char *s)
{
  int size = strlen(s);
  return string_ndup(s, size);
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_MEMORY_H_ */
