/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
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

#ifndef _KOALA_SYMTBL_H_
#define _KOALA_SYMTBL_H_

#include "hashmap.h"
#include "type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* symbol table */
typedef struct _symtbl {
    /* hash table for saving symbols */
    HashMap table;
} SymTbl, *SymTblRef;

/* symbol kind */
typedef enum symbolkind {
    // clang-format off
    SYM_CONST   = 1,   /* constant           */
    SYM_VAR     = 2,   /* variable           */
    SYM_FUNC    = 3,   /* function or method */
    SYM_CLASS   = 4,   /* class              */
    SYM_TRAIT   = 5,   /* trait              */
    SYM_ENUM    = 6,   /* enum               */
    SYM_LABEL   = 7,   /* enum label         */
    SYM_IFUNC   = 8,   /* interface method   */
    SYM_ANONY   = 9,   /* anonymous function */
    SYM_MOD     = 10,  /* (external) module  */
    SYM_MAX
    // clang-format on
} SymKind;

#define SYMBOL_HEAD     \
    HashMapEntry hnode; \
    SymKind kind;       \
    char *name;

typedef struct _Symbol {
    SYMBOL_HEAD
} Symbol, *SymbolRef;

typedef struct _VarSymbol {
    SYMBOL_HEAD
    TypeRef ty;
} VarSymbol, *VarSymbolRef;

SymTblRef stbl_new(void);
void stbl_free(SymTblRef stbl);

SymbolRef stbl_add_var(SymTblRef stbl, char *name, TypeRef ty);
SymbolRef stbl_get(SymTblRef stbl, char *name);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SYMTBL_H_ */
