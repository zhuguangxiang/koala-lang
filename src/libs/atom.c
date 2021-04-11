/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2020-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "buffer.h"
#include "hashmap.h"

typedef struct Atom {
    /* hashmap entry of Atom */
    HashMapEntry entry;
    /* the string length */
    int len;
    /* the string pointer */
    char *str;
} Atom;

static HashMap atom_table;

char *AtomNStr(char *s, int len)
{
    Atom key = { { NULL, MenHash(s, len) }, len, s };
    Atom *Atom = HashMapGet(&atom_table, &key);
    if (Atom) return Atom->str;

    Atom = MemAlloc(sizeof(*Atom) + len + 1);
    HashMapEntryInit(&Atom->entry, MenHash(s, len));
    Atom->len = len;
    Atom->str = (char *)(Atom + 1);
    strncpy(Atom->str, s, len);
    HashMapPutAbsent(&atom_table, Atom);
    return Atom->str;
}

char *AtomStr(char *s)
{
    return AtomNStr(s, strlen(s));
}

char *AtomVStr(int n, ...)
{
    char *s;
    BUF(buf);
    va_list args;

    va_start(args, n);
    BufVWrite(&buf, n, args);
    va_end(args);
    s = AtomStr(BUF_STR(buf));
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
    MemFree(entry);
}

void InitAtom(void)
{
    HashMapInit(&atom_table, _atom_equal_);
}

void FiniAtom(void)
{
    HashMapFini(&atom_table, _atom_free_, NULL);
}
