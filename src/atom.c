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
