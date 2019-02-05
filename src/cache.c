/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "cache.h"
#include "log.h"
#include "mem.h"

static Logger logger;

typedef struct cache_node {
  struct list_head free_node;
  char data[0];
} CacheNode;

int Init_Cache(Cache *cache, char *name, int objsize)
{
  cache->name = name;
  cache->objsize = objsize;
  cache->total = 0;
  cache->used = 0;
  init_list_head(&cache->free_list);
  return 0;
}

int Fini_Cache(Cache *cache, void (*finifunc)(void *, void *), void *arg)
{
  if (cache->used > 0) {
    Log_Error("cache-%s has %d objects used", cache->name, cache->used);
    assert(0);
    return -1;
  }

  int count = 0;
  struct list_head *p, *n;
  CacheNode *node;
  list_for_each_safe(p, n, &cache->free_list) {
    count++;
    list_del(p);
    node = container_of(p, CacheNode, free_node);
    if (finifunc)
      finifunc(node->data, arg);
    mm_free(node);
  }
  assert(count == cache->total);
  return 0;
}

void *Cache_Take(Cache *cache)
{
  CacheNode *node;
  struct list_head *first = list_first(&cache->free_list);
  if (first) {
    list_del(first);
    node = container_of(first, CacheNode, free_node);
  } else {
    node = mm_alloc(sizeof(CacheNode) + cache->objsize);
    init_list_head(&node->free_node);
    cache->total++;
  }
  cache->used++;
  return (void *)node->data;
}

void Cache_Restore(Cache *cache, void *obj)
{
  CacheNode *node = (CacheNode *)((char *)obj - sizeof(CacheNode));
  assert(list_unlinked(&node->free_node));
  list_add_tail(&node->free_node, &cache->free_list);
  cache->used--;
  assert(cache->used >= 0);
}
