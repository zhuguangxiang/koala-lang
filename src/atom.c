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

#include "hashmap.h"
#include "strbuf.h"

typedef struct atom {
  /* hashmap entry of atom */
  HashMapEntry entry;
  /* the string length */
  int len;
  /* the string pointer */
  char *str;
} Atom;

static HashMap atomtbl;

char *atom_nstring(char *s, int len)
{
  Atom key = {{NULL, memhash(s, len)}, len, s};
  Atom *atom = hashmap_get(&atomtbl, &key);
  if (atom) {
    return atom->str;
  }

  atom = kmalloc(sizeof(*atom) + len + 1);
  hashmap_entry_init(&atom->entry, memhash(s, len));
  atom->len = len;
  atom->str = (char *)(atom + 1);
  strncpy(atom->str, s, len);
  hashmap_add(&atomtbl, atom);
  return atom->str;
}

char *atom(char *s)
{
  return atom_nstring(s, strlen(s));
}

char *atom_vstring(int n, ...)
{
  char *s;
  STRBUF(sbuf);
  va_list args;

  va_start(args, n);
  strbuf_vnappend(&sbuf, n, args);
  va_end(args);
  s = atom(strbuf_tostr(&sbuf));
  strbuf_fini(&sbuf);

  return s;
}

static int _atom_equal_cb_(void *e1, void *e2)
{
  Atom *a1 = e1;
  Atom *a2 = e2;
  if (a1->len != a2->len)
    return 0;
  return !strncmp(a1->str, a2->str, a1->len);
}

static void _atom_free_cb_(void *entry, void *data)
{
  kfree(entry);
}

void init_atom(void)
{
  hashmap_init(&atomtbl, _atom_equal_cb_);
}

void fini_atom(void)
{
  hashmap_fini(&atomtbl, _atom_free_cb_, NULL);
}
