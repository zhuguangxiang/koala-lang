/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
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
