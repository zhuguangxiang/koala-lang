/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "common.h"
#include "hashmap.h"
#include "ir.h"
#include "typespec.h"
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
    SYM_INTF,           /* interface  */
    SYM_ANONY,          /* anonymous  */
    SYM_TYPE_PARAM,     /* type param */
    SYM_MODULE,         /* module     */
    SYM_INSTANCE,       /* instance   */
    SYM_MAX,
} SymKind;

#define SYM_FLAGS_MUTABLE   (1 << 0)
#define SYM_FLAGS_PUBLIC    (1 << 1)
#define SYM_FLAGS_TAG_ONLY  (1 << 2)
#define SYM_FLAGS_TAG_VALUE (1 << 3)

#define SYMBOL_HEAD \
    HashMapEntry hnode; SymKind kind; int flags; int id; char *name; TypeSpec *ts; \
    HashMap *stbl; KlrValue *ir_val;

/* clang-format on */

typedef struct _Symbol {
    SYMBOL_HEAD
} Symbol;

typedef struct _Literal {
    int which;
#define LIT_INT  1
#define LIT_FLT  2
#define LIT_BOOL 3
#define LIT_STR  4
#define LIT_NONE 5
    int len;
    int sign;
    union {
        uint64_t ival;
        double fval;
        int bval;
        char *sval;
    };
} Literal;

typedef struct _VarSymbol {
    SYMBOL_HEAD
    int scope;
#define VAR_SCOPE_GLOBAL 1
#define VAR_SCOPE_LOCAL  2
#define VAR_SCOPE_PARAM  3
    Literal *lit;
} VarSymbol;

typedef struct _TypeParamSymbol {
    // ts is not used
    SYMBOL_HEAD
    // owner symbol
    Symbol *owner;
    // list of TypeSpec
    Vector *bound;
    // index in type-param list
    int index;
    // invariant/covariant
    int covariant;
} TypeParamSymbol;

typedef struct _ArgInfo {
    char *name;
    Symbol *sym;
    TypeSpec *ts;
    int dfl_val_idx;
} ArgInfo;

typedef struct _FuncSymbol {
    SYMBOL_HEAD
    /* annotation */
    char *ann;
    char *ann_key;
    /* return type */
    TypeSpec *ret;
    /* ArgInfo list */
    Vector *params;
    /* type params */
    Vector *tps;
    /* local variables */
    Vector *locals;
    /* stack size */
    int stack_size;
    /* code size */
    int code_size;
    /* codes */
    char *codes;
} FuncSymbol;

typedef struct _KlassSymbol {
    SYMBOL_HEAD
    /* type params */
    Vector *tps;
    /* ->TypeSpec */
    Vector *bases;
    /* fields */
    Vector *fields;
    /* functions */
    Vector *funcs;
    /* protos */
    Vector *protos;
    /* instance type */
    TypeSpec *instance_ts;
} KlassSymbol;

typedef struct _ModuleSymbol {
    SYMBOL_HEAD
    char *pkgname;
} ModuleSymbol;

/* List[int] -> _Z4Listi */
typedef struct _InstanceSymbol {
    SYMBOL_HEAD
    /* -> KlassSymbol */
    Symbol *origin;
    /* type param binding args (T: int) */
    Vector *tp_args;
    /* instance type(specialized) */
    TypeSpec *instance_ts;
    /* instance bases */
    Vector *bases;
} InstanceSymbol;

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

Symbol *stbl_add_var(HashMap *stbl, char *name, TypeSpec *ts, int flags);
Symbol *stbl_add_func(HashMap *stbl, char *name, Vector *tps, TypeSpec *ret,
                      Vector *params, int flags, char *ann, char *ann_key);
Symbol *stbl_add_klass(HashMap *stbl, char *name, int flags, int is_trait);
Symbol *stbl_add_type_param(HashMap *stbl, char *name, Symbol *owner);
Symbol *stbl_get(HashMap *stbl, char *name);
void stbl_show(HashMap *stbl);
void *get_symbol_by_id(int id);

Symbol *stbl_add_module(HashMap *stbl, char *path);
Symbol *find_or_add_instance(HashMap *stbl, Symbol *origin, Vector *tp_args);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SYMBOL_H_ */
