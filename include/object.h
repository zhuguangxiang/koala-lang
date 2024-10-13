/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "common.h"
#include "gc.h"
#include "hashmap.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */
#define OBJECT_HEAD GcObject ob_gc_obj; struct _TypeObject *ob_type;
/* clang-format on */

typedef struct _Object {
    OBJECT_HEAD
} Object;

#define OBJECT_HEAD_INIT(_type) \
    .ob_gc_obj = { GC_OBJECT_INIT(0, -1, GC_COLOR_BLACK) }, .ob_type = (_type)

#define INIT_OBJECT_HEAD(_ob, _type) (_ob)->ob_type = (_type)

#define OB_TYPE(_ob) (((Object *)(_ob))->ob_type)

#define IS_TYPE(ob, type) (OB_TYPE(ob) == type)

typedef struct _Value {
    union {
        int64_t ival;
        double fval;
        void *obj;
    };
    int tag;
} Value;

#define VAL_TAG_NONE   0 // none(null/nil) value
#define VAL_TAG_INT    1
#define VAL_TAG_FLOAT  2
#define VAL_TAG_OBJECT 3
#define VAL_TAG_ERROR  4 // nothing type

#define IS_OBJ(x)   ((x)->tag == VAL_TAG_OBJECT)
#define IS_INT(x)   ((x)->tag == VAL_TAG_INT)
#define IS_FLOAT(x) ((x)->tag == VAL_TAG_FLOAT)
#define IS_NONE(x)  ((x)->tag == VAL_TAG_NONE)
#define IS_ERROR(x) ((x)->tag == VAL_TAG_ERROR)

/* clang-format off */
#define obj_value(x)   (Value){ .tag = VAL_TAG_OBJECT, .obj  = (x) }
#define int_value(x)   (Value){ .tag = VAL_TAG_INT,    .ival = (int64_t)(x) }
#define float_value(x) (Value){ .tag = VAL_TAG_FLOAT,  .fval = (double)(x)  }
#define none_value     (Value){ .tag = VAL_TAG_NONE,   .ival = 0   }
#define error_value    (Value){ .tag = VAL_TAG_ERROR,  .ival = -1  }

// will throw exception, if type check failed.
#define as_obj(v)   ({ ASSERT(IS_OBJ(v)); (v)->obj; })
#define as_int(v)   ({ ASSERT(IS_INT(v)); (v)->ival; })
#define as_float(v) ({ ASSERT(IS_INT(v)); (v)->fval; })

#define to_obj(v)   ((v)->obj)
#define to_int(v)   ((v)->ival)
#define to_float(v) ((v)->fval)

/* clang-format on */

static inline void gc_mark_value(Value *val, Queue *que)
{
    if (IS_OBJ(val)) {
        gc_mark_obj(to_obj(val), que);
    }
}

typedef void (*GcMarkFunc)(Object *, Queue *);
typedef Value (*HashFunc)(Value *self);
typedef Value (*CmpFunc)(Value *lhs, Value *rhs);
typedef Value (*GetIterFunc)(Object *self);
typedef Value (*IterNextFunc)(Object *self);
typedef Value (*StrFunc)(Value *self);
typedef Object *(*AllocFunc)(struct _TypeObject *tp);
typedef int (*InitFunc)(Value *self, Value *args, int nargs, Object *names);
typedef void (*FiniFunc)(Object *self);
typedef Value (*CallFunc)(Value *self, Value *args, int nargs, Object *names);

typedef Value (*CFuncNoArgs)(Value *);
typedef Value (*CFuncOneArg)(Value *, Value *);
typedef Value (*CFuncVarArgs)(Value *, Value *, int);
typedef Value (*CFuncVarArgsNames)(Value *, Value *, int, Object *);

#define FIELD_F_PUBLIC 1
#define FIELD_F_READ   2
#define FIELD_F_WRITE  4

typedef struct _GetSetDef {
    /* name */
    const char *name;
    /* type descriptor */
    const char *desc;
    /* flags */
    int flags;
    /* get callback */
    Value (*get)(void);
    /* set callback */
    void (*set)(Value *);
} GetSetDef;

typedef struct _MemberDef {
    /* name */
    const char *name;
    /* type descriptor */
    const char *desc;
    /* type */
    int type;
#define MBR_T_BYTE   1
#define MBR_T_SHORT  2
#define MBR_T_INT    3
#define MER_T_LONG   4
#define MER_T_FLOAT  5
#define MER_T_DOUBLE 6
#define MBR_T_STRING 7
#define MBR_T_VALUE  8
#define MBR_T_OBJECT 9
    /* offset */
    int offset;
    /* flags */
    int flags;
} MemberDef;

typedef struct _MethodDef {
    /* The name of function/method */
    const char *name;
    /* The c func */
    void *cfunc;
    /* flags */
    int flags;
    /* The arguments types */
    const char *args_desc;
    /* The return type */
    const char *ret_desc;
} MethodDef;

/* Value fn(Value *self) */
#define METH_NO_ARGS 1
/* Value fn(Value *self, Value *oth) */
#define METH_ONE_ARG 2
/* Value fn(Value *self, Value *args, int nargs) */
#define METH_VAR_ARGS 3
/* Value fn(Value *self, Value *args, int nargs, Object *names) */
#define METH_VAR_NAMES 4

/*
typedef struct _NumberMethods {
    BinaryFunc nb_add;
    BinaryFunc nb_sub;
    BinaryFunc nb_mul;
    BinaryFunc nb_div;
    BinaryFunc nb_mod;

    BinaryFunc nb_and;
    BinaryFunc nb_or;
    BinaryFunc nb_xor;

    BinaryFunc nb_shl;
    BinaryFunc nb_shr;
    BinaryFunc nb_ushr;
} NumberMethods;

typedef struct _SeqMapMethods {
    LenFunc sm_len;
    BinaryFunc sm_add;
    ContainFunc sm_contain;
    RepeatFunc sm_repeat;
    BinaryFunc sm_get_item;
    BinaryFunc sm_gets;
    BinaryFunc sm_sets;
} SeqMapMethods;
*/

typedef struct _SymbolEntry {
    HashMapEntry hnode;
    const char *key;
    int len;
    Object *obj;
} SymbolEntry;

void init_symbol_table(HashMap *map);
Object *table_find(HashMap *map, const char *name, int len);
void table_add_object(HashMap *map, const char *name, Object *obj);

#define TP_FLAGS_CLASS    (1 << 0)
#define TP_FLAGS_TRAIT    (1 << 1)
#define TP_FLAGS_ABSTRACT (1 << 2)
#define TP_FLAGS_PUBLIC   (1 << 3)
#define TP_FLAGS_FINAL    (1 << 4)
#define TP_FLAGS_META     (1 << 5)
#define TP_FLAGS_HEAP     (1 << 6)
#define TP_FLAGS_READY    (1 << 7)
#define TP_FLAGS_READYING (1 << 8)

typedef struct _TraitObject {
    /* type name */
    char *name;
    /* one of TP_FLAGS_XXX */
    int flags;
    /* module */
    Object *owner;
    /* interfaces */
    Vector *intfs;
} TraitObject;

typedef struct _TypeObject {
    OBJECT_HEAD
    /* type name */
    char *name;
    /* one of TP_FLAGS_XXX */
    int flags;

    /* descriptor of call() */
    char *desc;

    /* module */
    Object *module;

    /* gc mark function */
    GcMarkFunc mark;

    /* allocate function */
    AllocFunc alloc;
    /* init function */
    InitFunc init;
    /* fini function */
    FiniFunc fini;

    /* hashable */
    HashFunc hash;
    /* -1: less than; 0 equal; 1 greater than */
    CmpFunc cmp;
    /* printable */
    StrFunc str;

    /* callable */
    CallFunc call;

    /* iterable */
    GetIterFunc iter;
    IterNextFunc next;

    /* attribute get/set */
    // AttrGetFunc attr_get;
    // AttrSetFunc attr_set;

    /* members */
    MemberDef *members;
    /* getsets */
    GetSetDef *getsets;
    /* method definitions */
    MethodDef *methods;

    /* base class and traits */
    struct _TypeObject *base;
    struct _TraitObject **traits;

    /* fields(getset/member/var/value) */
    Vector *fields;
    /* functions(parent and self methods) */
    Vector *funcs;
    /* symbol table */
    HashMap map;
} TypeObject;

extern TypeObject type_type;
extern TypeObject base_type;
extern TypeObject none_type;
extern TypeObject int_type;
extern TypeObject float_type;

/* common header of CFuncObject & CodeObject */
#define FUNCTION_HEAD \
    OBJECT_HEAD \
    Object *module; /* module */ \
    TypeObject *cls; /* class, can be null */

typedef struct _FuncObject {
    FUNCTION_HEAD
} FuncObject;

/* call site cache */
// https://en.wikipedia.org/wiki/Inline_caching
// https://bernsteinbear.com/blog/inline-caching/
typedef struct _MethodSite {
    TypeObject *type;
    const char *fname;
    Object *method;
} MethodSite;

int type_ready(TypeObject *tp, Object *module);
TypeObject *object_typeof(Value *val);

static inline CallFunc object_callable(Value *val)
{
    TypeObject *tp = object_typeof(val);
    if (!tp) return NULL;
    return tp->call;
}

/* clang-format off */
#define _compare_result(v) ({   \
    int r;                      \
    if (v > 0) { r = 1;         \
    } else if (v < 0) { r = -1; \
    } else { r = 0;             \
    }                           \
    r;                          \
})
/* clang-format on */

Value object_call(Value *self, Value *args, int nargs, Object *names);
Value methodsite_call(MethodSite *ms, Value *args, int nargs, Object *names);
Object *kl_lookup_method(Value *obj, const char *fname);
int kl_parse_kwargs(Value *args, int nargs, Object *names, int npos, const char **kws,
                    ...);
Value object_str(Value *self);
Object *type_lookup_object(Object *tp, const char *name, int len);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
