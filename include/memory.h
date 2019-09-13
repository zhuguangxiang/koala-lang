/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

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
