/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#include "buffer.h"
#include "hashmap.h"

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
typedef struct _Atom Atom;

struct _Atom {
    /* hashmap entry of the atom string */
    HashMapEntry entry;
    /* the string length */
    int len;
    /* the string data, its size is len + 1 */
    char *str;
};

/* atom string table */
static HashMap atom_tbl;

char *atom_nstr(char *s, int len)
{
    unsigned int hash = mem_hash(s, len);
    Atom key = { { NULL, hash }, len, s };
    Atom *atom = hashmap_get(&atom_tbl, &key);
    if (atom) return atom->str;

    atom = mm_alloc(sizeof(*atom) + len + 1);
    hashmap_entry_init(atom, hash);
    atom->len = len;
    atom->str = (char *)(atom + 1);
    strncpy(atom->str, s, len);
    hashmap_put_only(&atom_tbl, atom);
    return atom->str;
}

char *atom(char *s)
{
    return atom_nstr(s, strlen(s));
}

char *atom_concat(int n, ...)
{
    char *s;
    BUF(buf);

    va_list args;
    va_start(args, n);
    while (n-- > 0) {
        s = va_arg(args, char *);
        buf_write_str(&buf, s);
    }
    va_end(args);

    s = atom(BUF_STR(buf));
    FINI_BUF(buf);

    return s;
}

static int _atom_equal_(void *e1, void *e2)
{
    Atom *a1 = e1;
    Atom *a2 = e2;
    if (a1->len != a2->len) return 0;
    return !strncmp(a1->str, a2->str, a1->len);
}

static void _atom_free_(void *entry, void *data)
{
    UNUSED(data);
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

#ifdef __cplusplus
}
#endif
