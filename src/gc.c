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

#include "gc.h"
#include "object.h"

#define GC_STOPPED  1
#define GC_MARKING  2
#define GC_SWEEPING 3

static int state = GC_STOPPED;
static LIST_HEAD(oblist);
static LIST_HEAD(garbage);

void *gcmalloc(int size)
{
  size += sizeof(GCHeader);
  GCHeader *ob = kmalloc(size);
  init_list_head(&ob->link);
  list_add(&ob->link, &oblist);
  return (void *)(ob + 1);
}

void gcfree(void *ptr)
{
  GCHeader *ob = (GCHeader *)ptr - 1;
  list_del(&ob->link);
  kfree(ob);
}

void gc(void)
{
  expect(state == GC_STOPPED);

  state = GC_MARKING;

  GCHeader *gch;
  struct list_head *p, *n;
  list_for_each_safe(p, n, &oblist) {
    gch = (GCHeader *)p;
    if (!gch->marked) {
      list_del(p);
      list_add_tail(p, &garbage);
    }
  }

  state = GC_SWEEPING;

  Object *ob;
  list_for_each(p, &garbage) {
    ob = (Object *)((GCHeader *)p + 1);
    if (OB_TYPE(ob)->clean != NULL)
      OB_TYPE(ob)->clean(ob);
  }

  list_for_each_safe(p, n, &garbage) {
    ob = (Object *)((GCHeader *)p + 1);
    expect(ob->ob_refcnt == 0);
    gcfree(ob);
  }

  state = GC_STOPPED;
}

int isgc(void)
{
  return state != GC_STOPPED;
}
