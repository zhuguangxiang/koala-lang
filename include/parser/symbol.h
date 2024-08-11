/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "common.h"
#include "hashmap.h"
#include "typedesc.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */
typedef enum _SymKind {
    SYM_UNK,            /* unknown    */
    SYM_VAR,            /* variable   */
    SYM_FUNC,           /* function   */
    SYM_CLASS,          /* class      */
    SYM_TRAIT,          /* trait      */
    SYM_FIELD,          /* field      */
    SYM_PROTO,          /* func proto */
    SYM_ANONY,          /* anony func */
    SYM_PACKAGE,        /* package    */
    SYM_TYPE_PARAM,     /* type param */
    SYM_MAX,
} SymKind;

#define SYMBOL_HEAD \
    HashMapEntry hnode; SymKind kind; char *name; TypeDesc *desc; HashMap *stbl;

/* clang-format on */

typedef struct _Symbol {
    SYMBOL_HEAD
} Symbol;

typedef struct _VarSymbol {
    SYMBOL_HEAD
    int scope;
#define VAR_SCOPE_GLOBAL 1
#define VAR_SCOPE_LOCAL  2
#define VAR_SCOPE_PARAM  3
} VarSymbol;

static inline int __symbol_equal__(Symbol *s1, Symbol *s2)
{
    return !strcmp(s1->name, s2->name);
}

static inline HashMap *stbl_new(void)
{
    HashMap *stbl = mm_alloc_obj_fast(stbl);
    hashmap_init(stbl, (HashMapEqualFunc)__symbol_equal__);
    return stbl;
}

void __symbol_free__(Symbol *sym, void *arg);

static inline void stbl_free(HashMap *stbl)
{
    if (!stbl) return;
    hashmap_fini(stbl, (HashMapVisitFunc)__symbol_free__, NULL);
    mm_free(stbl);
}

void stbl_show(HashMap *stbl);
Symbol *stbl_add_var(HashMap *stbl, char *name, TypeDesc *desc);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SYMBOL_H_ */
