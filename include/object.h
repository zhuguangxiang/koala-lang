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

#define VAL_TAG_OBJ   1
#define VAL_TAG_INT   2
#define VAL_TAG_FLOAT 3
#define VAL_TAG_NONE  4 // no value(null or void)
#define VAL_TAG_ERROR 5 // nothing type

#define IS_OBJECT(x) ((x)->tag == VAL_TAG_OBJ)
#define IS_INT(x)    ((x)->tag == VAL_TAG_INT)
#define IS_FLOAT(x)  ((x)->tag == VAL_TAG_FLOAT)
#define IS_NONE(x)   ((x)->tag == VAL_TAG_NONE)
#define IS_ERROR(x)  ((x)->tag == VAL_TAG_ERROR)

/* clang-format off */
#define ObjectValue(x) (Value){ .tag = VAL_TAG_OBJ,   .obj  = (x) }
#define IntValue(x)    (Value){ .tag = VAL_TAG_INT,   .ival = (x) }
#define FloatValue(x)  (Value){ .tag = VAL_TAG_FLOAT, .fval = (x) }
#define NoneValue      (Value){ .tag = VAL_TAG_NONE }
#define ErrorValue     (Value){ .tag = VAL_TAG_ERROR }

#define value_as_object(v) ({ ASSERT(IS_OBJECT(v)); (v)->obj; })
#define value_as_int(v) ({ ASSERT(IS_INT(v)); (v)->ival; })
#define value_as_float(v) ({ ASSERT(IS_INT(v)); (v)->fval; })

/* clang-format on */

static inline void gc_mark_value(Value *val, Queue *que)
{
    if (IS_OBJ(val)) {
        gc_mark_obj(val->obj, que);
    }
}

typedef HashMap KeywordMap;

typedef struct _KeywordEntry {
    HashMapEntry hnode;
    const char *key;
    Value val;
} KeywordEntry;

typedef void (*GcMarkFunc)(Object *, Queue *);
typedef Value (*HashFunc)(Value *self);
typedef Value (*CmpFunc)(Value *lhs, Value *rhs);
typedef Value (*GetIterFunc)(Value *self);
typedef Value (*IterNextFunc)(Value *self);
typedef Value (*StrFunc)(Value *self);
typedef Object *(*AllocFunc)(TypeObject *tp);
typedef int (*InitFunc)(Object *self, Value *args, int nargs, KeywordMap *kwargs);
typedef void (*FiniFunc)(Object *self);
typedef Value (*CallFunc)(Value *self, Value *args, int nargs, KeywordMap *kwargs);

typedef Value (*CFuncNoArgs)(Value *);
typedef Value (*CFuncOneArg)(Value *, Value *);
typedef Value (*CFuncVarArgs)(Value *, Value *, int);
typedef Value (*CFuncVarArgsNames)(Value *, Value *, int, KeywordMap *);

typedef struct _MethodDef {
    /* The name of function/method */
    char *name;
    /* The c func */
    void *cfunc;
    /* flags */
    int flags;
    /* The arguments types */
    char *args_desc;
    /* The return type */
    char *ret_desc;
} MethodDef;

/* Value fn(Value *self) */
#define METH_NO_ARGS 1
/* Value fn(Value *self, Value *oth) */
#define METH_ONE_ARG 2
/* Value fn(Value *self, Value *args, int nargs) */
#define METH_VAR_ARGS 3
/* Value fn(Value *self, Value *args, int nargs, Tuple *names) */
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

#define TP_FLAGS_CLASS    (1 << 0)
#define TP_FLAGS_TRAIT    (1 << 1)
#define TP_FLAGS_ABSTRACT (1 << 2)
#define TP_FLAGS_PUBLIC   (1 << 3)
#define TP_FLAGS_FINAL    (1 << 4)
#define TP_FLAGS_META     (1 << 5)
#define TP_FLAGS_HEAP     (1 << 6)
#define TP_FLAGS_READY    (1 << 7)

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
    /* object size */
    int size;

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
    CmpFunc cmp;
    /* printable */
    StrFunc str;

    /* callable */
    CallFunc call;

    /* iterable */
    GetIterFunc iter;
    IterNextFunc next;

    /* method definitions */
    MethodDef *methods;

    /* base class and traits */
    struct _TypeObject *base;
    struct _TraitObject **traits;

    /* attributes(fields) */
    Vector *attrs;

    /* functions(parent and self methods) */
    Vector *funcs;

    /* dynamic functions table */
    KeywordMap *vtbl;

} TypeObject;

extern TypeObject type_type;
extern TypeObject base_type;

TypeObject *object_type(Value *val);
Object *object_generic_alloc(TypeObject *tp);

static inline CallFunc object_callable(Value *val)
{
    TypeObject *tp = object_type(val);
    if (!tp) return NULL;
    return tp->call;
}

Value object_call(Value *self, Value *args, int nargs, KeywordMap *kwargs);

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

Object *object_lookup_method(Value *obj, const char *fname);

/* call site cache */
// https://en.wikipedia.org/wiki/Inline_caching
// https://bernsteinbear.com/blog/inline-caching/
typedef struct _SiteMethod {
    TypeObject *type;
    const char *fname;
    Object *method;
} SiteMethod;

Value object_call_site_method(SiteMethod *sm, Value *args, int nargs, KeywordMap *kwargs);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
