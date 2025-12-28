/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "common.h"
#include "hashmap.h"
#include "ir.h"
#include "typedesc.h"
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
    SYM_FIELD,          /* field      */
    SYM_PROTO,          /* proto      */
    SYM_ANONY,          /* anonymous  */
    SYM_PACKAGE,        /* package    */
    SYM_TYPE_PARAM,     /* type param */
    SYM_MAX,
} SymKind;

#define SYM_FLAGS_VAR_VALUE (1 << 0)
#define SYM_FLAGS_MUTABLE   (1 << 1)
#define SYM_FLAGS_PUBLIC    (1 << 2)
#define SYM_FLAGS_FINAL     (1 << 3)
#define SYM_FLAGS_STATIC    (1 << 4)
#define SYM_FLAGS_TAG_ONLY  (1 << 5)
#define SYM_FLAGS_TAG_VALUE (1 << 6)

#define SYMBOL_HEAD \
    HashMapEntry hnode; SymKind kind; int flags; int id; char *name; TypeDesc *desc; TypeSpec *ts; \
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
    // list of TypeSpec
    Vector *bound;
    // index in type-param list
    int index;
} TypeParamSymbol;

typedef struct _ArgInfo {
    char *name;
    TypeSpec *ts;
    int dfl_val_idx;
} ArgInfo;

typedef struct _FuncSymbol {
    SYMBOL_HEAD
    /* annotation */
    char *ann;
    char *ann_key;
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
    /* bases */
    Vector *bases;
    /* fields */
    Vector *fields;
    /* functions */
    Vector *funcs;
    /* protos */
    Vector *protos;
} KlassSymbol;

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
Symbol *stbl_add_klass(HashMap *stbl, char *name, int flags);
Symbol *stbl_add_trait(HashMap *stbl, char *name, int flags);
Symbol *stbl_add_type_param(HashMap *stbl, char *name);
Symbol *stbl_get(HashMap *stbl, char *name);
void stbl_show(HashMap *stbl);
void *get_symbol_by_id(int id);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SYMBOL_H_ */
