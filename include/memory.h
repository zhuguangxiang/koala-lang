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

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* slab contains #n objects */
struct slab {
  /* linked in cache's full list */
  struct slab *next;
  /* free object list */
  void *freelist;
  /* allocated objects of the slab */
  int used;
};

/* fixed size object cache */
struct cache {
  /* cache name of object */
  char *name;
  /* object size */
  int objsze;
  /* allocated count */
  int allocated;
  /* full slab list */
  struct slab *full;
  /* partial slab(current) */
  struct slab *partial;
};

/*
 * Initialze an object cache.
 *
 * self    - The cache to be initialized.
 * name    - The cache name for debug.
 * objsize - The object size in the cache.
 *
 * Returns nothing.
 */
void kcache_init(struct cache *self, char *name, int objsize);

/*
 * Free an object cache and its allocated memory.
 *
 * self - The cache to be freed.
 *
 * Returns nothing.
 */
void kcache_free(struct cache *self);

/*
 * Create a new object cache.
 *
 * name    - The cache name for debug.
 * objsize - The object size in the cache.
 *
 * Returns a new cache or null if memory allocation failed.
 */
struct cache *kcache_create(char *name, int objsize);

/*
 * Destroy an object cache and free its allocated memory.
 *
 * self - The cache to be destroyed.
 *
 * Returns nothing.
 */
void kcache_destroy(struct cache *self);

/*
 * Alloc an object from its cache.
 *
 * self - The cache to allocate objects.
 *
 * Returns an object or null if memory allocation failed.
 */
void *kcache_alloc(struct cache *self);

/*
 * Restore an object to its cache.
 *
 * self - The cache to restore objects.
 * obj  - The object to be restored.
 *
 * Returns nothing.
 */
void kcache_restore(struct cache *self, void *obj);

/*
 * Stat a cache usage.
 *
 * self - The cache to stat.
 *
 * Returns nothing.
 */
void kcache_stat(struct cache *self);

/*
 * allocate memory for any size.
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
void kfree(void *ptr);

/*
 * stat memory usage with 'kmalloc' and 'kfree'.
 *
 * Returns nothing.
 */
void stat(void);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_MEMORY_H_ */
