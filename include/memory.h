/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
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
  assert(ptr);     \
  __kfree(ptr);    \
  ptr = NULL;      \
})

/*
 * stat memory usage with 'kmalloc' and 'kfree'.
 *
 * Returns nothing.
 */
void kstat(void);

/*
 * duplicate c cstring, replace of strndup.
 *
 * s    - The origin string.
 * size - The size to copy.
 *
 * Returns a copied string with '\0'.
 */
static inline char *string_ncopy(char *s, size_t size)
{
  char *str = kmalloc(size + 1);
  memcpy(str, s, size);
  return str;
}

/*
 * duplicate c cstring, with extra available size.
 *
 * msize - The copied string memory size.
 * s     - The origin string.
 * size  - The size to copy.
 *
 * Returns a copied string with '\0'.
 */
static inline char *string_ncopy_size(size_t msize, char *s, size_t size)
{
  assert(msize >= size);
  char *str = kmalloc(msize + 1);
  memcpy(str, s, size);
  return str;
}

/*
 * duplicate c cstring, replace of strdup
 *
 * s    - The origin string.
 *
 * Returns a copied string with '\0'.
 */
static inline char *string_copy(char *s)
{
  int size = strlen(s);
  return string_ncopy(s, size);
}

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_MEMORY_H_ */
