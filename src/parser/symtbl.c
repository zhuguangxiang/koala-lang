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

#include "symtbl.h"

#ifdef __cplusplus
extern "C" {
#endif

static int symbol_equal(void *k1, void *k2)
{
    SymbolRef s1 = k1;
    SymbolRef s2 = k2;
    return k1 == k2 || !strcmp(s1->name, s2->name);
}

SymTblRef stbl_new(void)
{
    SymTblRef stbl = MemAlloc(sizeof(*stbl));
    HashMapInit(&stbl->table, symbol_equal);
    return stbl;
}

static void _symbol_free_(void *e, void *arg)
{
    SymbolRef sym = e;
    MemFree(sym);
}

void stbl_free(SymTblRef stbl)
{
    if (stbl == NULL) return;
    HashMapFini(&stbl->table, _symbol_free_, NULL);
    MemFree(stbl);
}

SymbolRef stbl_add_var(SymTblRef stbl, char *name, TypeRef ty)
{
    VarSymbolRef sym = MemAlloc(sizeof(*sym));
    HashMapEntryInit(sym, StrHash(name));
    sym->kind = SYM_VAR;
    sym->name = name;
    sym->ty = ty;
    if (HashMapPutAbsent(&stbl->table, sym) < 0) {
        printf("add symbol '%s' failed", sym->name);
    }
    return (SymbolRef)sym;
}

SymbolRef stbl_get(SymTblRef stbl, char *name)
{
    if (stbl == NULL) return NULL;
    Symbol key = { .name = name };
    HashMapEntryInit(&key, StrHash(name));
    return HashMapGet(&stbl->table, &key);
}

#ifdef __cplusplus
}
#endif
