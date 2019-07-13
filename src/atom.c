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

#include "hashmap.h"
#include "strbuf.h"

struct atom {
  /* hashmap entry of atom */
  struct hashmap_entry entry;

  /* the string length */
  int len;

  /* the string pointer */
  char *str;
};

static struct hashmap atom_map;

char *atom_string(char *s, int len)
{
  struct atom key = {{NULL, memhash(s, len)}, len, s};
  struct atom *atom = hashmap_get(&atom_map, &key);
  if (atom) {
    return atom->str;
  }

  atom = kmalloc(sizeof(*atom) + len + 1);
  hashmap_entry_init(&atom->entry, memhash(s, len));
  atom->len = len;
  atom->str = (char *)(atom + 1);
  strncpy(atom->str, s, len);
  hashmap_add(&atom_map, atom);
  return atom->str;
}

char *atom(char *s)
{
  return atom_string(s, strlen(s));
}

char *atom_nstring(int n, ...)
{
  char *s;
  STRBUF(sbuf);
  va_list args;

  va_start(args, n);
  strbuf_vnappend(&sbuf, n, args);
  va_end(args);
  s = atom(strbuf_tostr(&sbuf));
  strbuf_free(&sbuf);

  return s;
}

static int __atom_cmp_cb__(void *e1, void *e2)
{
  struct atom *a1 = e1;
  struct atom *a2 = e2;
  if (a1->len != a2->len)
    return -1;
  return strncmp(a1->str, a2->str, a1->len);
}

static void __atom_free_cb__(void *entry, void *data)
{
  kfree(entry);
}

void atom_initialize(void)
{
  hashmap_init(&atom_map, __atom_cmp_cb__);
}

void atom_destroy(void)
{
  hashmap_free(&atom_map, __atom_free_cb__, NULL);
}
