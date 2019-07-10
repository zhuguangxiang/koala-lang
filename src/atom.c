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

#include <stdarg.h>
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

char *atom_nstring(char *s, int len)
{
  struct atom key = {{NULL, strhash(s)}, len, s};
  struct hashmap_entry *entry = hashmap_get(&atom_map, &key.entry);
  if (entry) {
    return ((struct atom *)entry)->str;
  }

  struct atom *atom = kmalloc(sizeof(*atom) + len + 1);
  hashmap_entry_init(&atom->entry, strhash(s));
  atom->len = len;
  atom->str = (char *)(atom + 1);
  strcpy(atom->str, s);
  hashmap_add(&atom_map, &atom->entry);
  return atom->str;
}

char *atom_string(char *s)
{
  return atom_nstring(s, strlen(s));
}

char *atom_vstring(int n, ...)
{
  STRBUF(sbuf);
  char *s;
  va_list args;

  va_start(args, n);
  while (n-- > 0) {
    s = va_arg(args, char *);
    strbuf_append(&sbuf, s);
  }
  va_end(args);

  s = atom_string(strbuf_tostr(&sbuf));
  strbuf_free(&sbuf);
  return s;
}

static int __atom_cmp_fn__(void *e1, void *e2)
{
  struct atom *a1 = e1;
  struct atom *a2 = e2;
  return strcmp(a1->str, a2->str);
}

static void __atom_free_fn__(struct hashmap_entry *e, void *data)
{
  kfree(e);
}

void atom_init(void)
{
  hashmap_init(&atom_map, __atom_cmp_fn__);
}

void atom_free(void)
{
  hashmap_free(&atom_map, __atom_free_fn__, NULL);
}
