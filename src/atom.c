/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
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

static struct hashmap atomtbl;

char *atom_nstring(char *s, int len)
{
  struct atom key = {{NULL, memhash(s, len)}, len, s};
  struct atom *atom = hashmap_get(&atomtbl, &key);
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
  strbuf_free(&sbuf);

  return s;
}

static int _atom_cmp_cb_(void *e1, void *e2)
{
  struct atom *a1 = e1;
  struct atom *a2 = e2;
  if (a1->len != a2->len)
    return -1;
  return strncmp(a1->str, a2->str, a1->len);
}

static void _atom_free_cb_(void *entry, void *data)
{
  kfree(entry);
}

void atom_init(void)
{
  hashmap_init(&atomtbl, _atom_cmp_cb_);
}

void atom_fini(void)
{
  hashmap_free(&atomtbl, _atom_free_cb_, NULL);
}
