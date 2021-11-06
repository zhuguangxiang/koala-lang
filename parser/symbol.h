/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "util/hashmap.h"
#include "util/typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _SymKind SymKind;
typedef struct _Symbol Symbol;
typedef struct _Symbol VarSymbol;
typedef struct _PackSymbol PackSymbol;
typedef struct _FuncSymbol FuncSymbol;
typedef struct _ParamTypeSymbol ParamTypeSymbol;

/* clang-format off */
enum _SymKind {
    SYM_LET = 1,    /* let                */
    SYM_VAR,        /* variable           */
    SYM_FUNC,       /* function or method */
    SYM_CLASS,      /* class              */
    SYM_TRAIT,      /* trait              */
    SYM_ENUM,       /* enum               */
    SYM_LABEL,      /* enum label         */
    SYM_IFUNC,      /* interface method   */
    SYM_ANONY,      /* anonymous function */
    SYM_PACKAGE,    /* package            */
    SYM_PARAM_TYPE, /* parameter type     */
    SYM_MAX,
};

#define SYMBOL_HEAD \
    HashMapEntry hnode; SymKind kind; char *name; \
    TypeDesc *type; HashMap *owner;

/* clang-format on */

struct _Symbol {
    SYMBOL_HEAD
};

struct _PackSymbol {
    SYMBOL_HEAD
    char *path;
    HashMap *stbl;
};

struct _FuncSymbol {
    SYMBOL_HEAD
    /* local symbol table */
    HashMap *stbl;
    /* local varibles */
    Vector *locvec;
    /* free variables */
    Vector *freevec;
};

struct _ParamTypeSymbol {
    SYMBOL_HEAD
    /* bounded type symbols */
    Vector *typesyms;
};

HashMap *stbl_new(void);
void stbl_free(HashMap *stbl);
Symbol *stbl_get(HashMap *stbl, char *name);
Symbol *stbl_add_let(HashMap *stbl, char *name, TypeDesc *desc);
Symbol *stbl_add_var(HashMap *stbl, char *name, TypeDesc *desc);
Symbol *stbl_add_func(HashMap *stbl, char *name);
Symbol *stbl_add_param_type(HashMap *stbl, char *name);
Symbol *stbl_add_package(HashMap *stbl, char *path, char *name);
void free_symbol(Symbol *sym);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SYMBOL_H_ */
