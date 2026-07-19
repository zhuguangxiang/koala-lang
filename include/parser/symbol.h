/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "hashmap.h"
#include "ir.h"
#include "typespec.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */
typedef enum _SymKind {
    SYM_UNK,                /* unknown    */
    SYM_VAR,                /* variable   */
    SYM_FUNC,               /* function   */
    SYM_CLASS,              /* class      */
    SYM_TRAIT,              /* trait      */
    SYM_ANONY,              /* anonymous  */
    SYM_TYPE_PARAM,         /* type param */
    SYM_PACKAGE,            /* package    */
    SYM_INSTANCE,           /* instance   */
    SYM_INHERITED,          /* inherited from trait */
    SYM_SHADOW_VAR,         /* shadow var */
    SYM_IMPORTED,           /* imported symbol */
    SYM_MAX,
} SymKind;

#define SYM_FLAGS_MUTABLE   (1 << 0)
#define SYM_FLAGS_CONST     (1 << 1)
#define SYM_FLAGS_PUBLIC    (1 << 2)
#define SYM_FLAGS_TAG_ONLY  (1 << 3)
#define SYM_FLAGS_TAG_VALUE (1 << 4)
#define SYM_FLAGS_EXT       (1 << 5)
#define SYM_FLAGS_MAGIC     (1 << 6)
#define SYM_FLAGS_GENERIC   (1 << 7)
#define SYM_FLAGS_NATIVE    (1 << 8)

#define SYM_UNRESOLVED   0
#define SYM_RESOLVED     1
#define SYM_RESOLVING    2

#define SYMBOL_HEAD \
    HashMapEntry hnode; SymKind kind; short flags; short status; int id; char *name; \
    char *path; TypeSpec *ts; HashMap *stbl; void *arg; void *parent; void *ps; \
    KlrValue *ir_val;

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
#define VAR_SCOPE_FIELD  4
    // default value for parameters
    Literal *lit;
    // for instance field, point to the origin field symbol
    // void *origin;
} VarSymbol;

typedef struct _ShadowVarSymbol {
    SYMBOL_HEAD
    HashMap *owner;
    Symbol *origin;
    int is_null;
} ShadowVarSymbol;

typedef struct _TypeParamSymbol {
    // ts is not used
    SYMBOL_HEAD
    // owner symbol
    Symbol *owner;
    // which one
    int which;
#define TP_NORMAL 0
#define TP_CONST  1
#define TP_INFER  2
    // index in type-param list
    int index;
    union {
        // list of TypeSpec
        Vector bound;
        // constant type
        TypeSpec *const_type;
    };
} TypeParamSymbol;

typedef struct _ArgInfo {
    char *name;
    Symbol *sym;
    TypeSpec *ts;
    int has_dfl_val;
    Literal *dfl_val;
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
    Vector tps;
    /* code index */
    int code_index;
    // for instance method, point to the origin method symbol
    // void *origin;
} FuncSymbol;

typedef struct _IntfEntry {
    /* key */
    Symbol *trait;
    /* all methods(self + inherited) */
    Vector methods;
    /* self index in intf_table */
    int index;
    /* upcast parents */
    Vector parents;
} IntfEntry;

typedef struct _KlassSymbol {
    SYMBOL_HEAD
    /* type params */
    Vector tps;
    /* ->TypeSpec */
    Vector bases;
    /* fields */
    Vector *fields;
    /* functions */
    Vector *funcs;
    /* instance type */
    TypeSpec *instance_ts;
    /* primary inheritance path */
    Vector pip;
    /* linear order */
    Vector lro;
    /* second chain map */
    Vector scm;
    /* interface table */
    Vector intf_table;
    /* __init__ function */
    Symbol *__init__;
    /* klc entry */
    void *klc_entry;
} KlassSymbol;

typedef struct _PkgSymbol {
    SYMBOL_HEAD
    // char *pkgname;
} PkgSymbol;

typedef struct _ImportedSymbol {
    SYMBOL_HEAD
    Symbol *origin;
} ImportedSymbol;

/* List[int] -> _Z4Listi */
typedef struct _InstanceSymbol {
    SYMBOL_HEAD
    /* -> KlassSymbol */
    Symbol *origin;
    /* type param binding args (T: int) */
    Vector *tp_args;
    /* instance type(generic_ref) */
    TypeSpec *instance_ts;
    /* instance bases */
    Vector *bases;
} InstanceSymbol;

typedef struct _InheritedFunc {
    SYMBOL_HEAD
    FuncSymbol *origin;
} InheritedFunc;

static inline int __symbol_equal__(Symbol *s1, Symbol *s2) { return !strcmp(s1->name, s2->name); }

static inline HashMap *stbl_new(void)
{
    HashMap *stbl = mm_alloc_obj_fast(stbl);
    hashmap_init(stbl, (HashMapEqualFunc)__symbol_equal__);
    return stbl;
}

static inline void stbl_free(HashMap *stbl)
{
    if (!stbl) return;
    // only free the hashmap itself, all symbols are managed by global list
    hashmap_fini(stbl, NULL, NULL);
    mm_free(stbl);
}

void free_all_symbols(void);

Symbol *stbl_add(HashMap *stbl, Symbol *sym);
Symbol *stbl_add_var(HashMap *stbl, char *name, TypeSpec *ts, int flags);
Symbol *stbl_add_func(HashMap *stbl, char *name, TypeSpec *ret, Vector *params, int flags);
Symbol *stbl_add_inherited_func(HashMap *stbl, Symbol *sym);
KlassSymbol *stbl_add_klass(HashMap *stbl, char *name, int flags, int is_trait);
TypeParamSymbol *stbl_add_type_param(HashMap *stbl, char *name, Symbol *owner);
Symbol *stbl_add_shadow_var(HashMap *stbl, Symbol *origin, int is_null);
Symbol *stbl_add_imported(HashMap *stbl, Symbol *origin, char *name);
Symbol *stbl_remove(HashMap *stbl, char *name);

static inline void remove_shadow_var(ShadowVarSymbol *sym)
{
    if (!sym || !sym->owner) return;
    stbl_remove(sym->owner, sym->name);
}

Symbol *stbl_get(HashMap *stbl, char *name);
void stbl_show(HashMap *stbl);
void *get_symbol_by_id(int id);

PkgSymbol *stbl_add_pkg(HashMap *stbl, char *path);
InstanceSymbol *find_or_add_instance(HashMap *stbl, Symbol *origin, Vector *tp_args);

/*
Find the Least Upper Bound (LUB) for a set of types.
Example: LUB([int, float, int]) -> number
         LUB([list[int], tuple[int], range]) -> Sequence[int]
         LUB([int, str]) -> any
         LUB([]) -> any
*/
TypeSpec *find_lub(Vector *types);

static inline int is_magic_func(FuncSymbol *sym) { return (sym->flags & SYM_FLAGS_MAGIC) != 0; }

void build_intf_table(HashMap *stbl);
void dump_intf_table(HashMap *stbl);
int get_intf_index(Symbol *sym, TypeSpec *trait_ts);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_SYMBOL_H_ */
