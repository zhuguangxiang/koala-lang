/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "buffer.h"
#include "hashmap.h"

typedef struct _Atom {
    /* hashmap entry of Atom */
    HashMapEntry entry;
    /* the string length */
    int len;
    /* the string pointer */
    char *str;
} Atom, *AtomRef;

static HashMap atom_tbl;

char *atom_str(char *s, int len)
{
    Atom key = { { NULL, mem_hash(s, len) }, len, s };
    AtomRef Atom = hashmap_get(&atom_tbl, &key);
    if (Atom) return Atom->str;

    Atom = mm_alloc(sizeof(*Atom) + len + 1);
    hashmap_entry_init(&Atom->entry, mem_hash(s, len));
    Atom->len = len;
    Atom->str = (char *)(Atom + 1);
    strncpy(Atom->str, s, len);
    hashmap_put_absent(&atom_tbl, Atom);
    return Atom->str;
}

char *atom(char *s)
{
    return atom_str(s, strlen(s));
}

char *atom_vstr(int n, ...)
{
    char *s;
    BUF(buf);
    va_list args;

    va_start(args, n);
    buf_vwrite(&buf, n, args);
    va_end(args);
    s = atom(BUF_STR(buf));
    FINI_BUF(buf);

    return s;
}

static int _atom_equal_(void *e1, void *e2)
{
    AtomRef a1 = e1;
    AtomRef a2 = e2;
    if (a1->len != a2->len) return 0;
    return !strncmp(a1->str, a2->str, a1->len);
}

static void _atom_free_(void *entry, void *data)
{
    mm_free(entry);
}

void init_atom(void)
{
    hashmap_init(&atom_tbl, _atom_equal_);
}

void fini_atom(void)
{
    hashmap_fini(&atom_tbl, _atom_free_, NULL);
}
