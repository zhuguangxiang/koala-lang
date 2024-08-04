/**
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
#define OBJECT_HEAD GC_HEAD struct _TypeObject *ob_type;
/* clang-format on */

typedef struct _Object {
    OBJECT_HEAD
} Object;

#define OBJECT_HEAD_INIT(_type) \
    GC_HEAD_INIT(0, -1, GC_COLOR_BLACK), .ob_type = (_type)

#define INIT_OBJECT_HEAD(_ob, _type) (_ob)->ob_type = (_type)

#define OB_TYPE(_ob) ((_ob)->ob_type)

#define IS_TYPE(ob, type) (OB_TYPE(ob) == type)

typedef struct _Value {
    int tag;
    union {
        int64_t ival;
        double fval;
        Object *obj;
    };
} Value;

#define VAL_TAG_OBJ  1
#define VAL_TAG_INT  2
#define VAL_TAG_FLT  3
#define VAL_TAG_NONE 4
#define VAL_TAG_ERR  5

#define IS_OBJECT(x) ((x)->tag == VAL_TAG_OBJ)
#define IS_INT(x)    ((x)->tag == VAL_TAG_INT)
#define IS_FLOAT(x)  ((x)->tag == VAL_TAG_FLT)
#define IS_NONE(x)   ((x)->tag == VAL_TAG_NONE)
#define IS_ERROR(x)  ((x)->tag == VAL_TAG_ERR)

/* clang-format off */
#define ObjectValue(x)  (Value){ .tag = VAL_TAG_OBJ, .obj = (x)  }
#define IntValue(x)     (Value){ .tag = VAL_TAG_INT, .ival = (x) }
#define FloatValue(x)   (Value){ .tag = VAL_TAG_FLT, .fval = (x) }
#define NoneValue       (Value){ .tag = VAL_TAG_NONE }
#define ErrorValue      (Value){ .tag = VAL_TAG_ERR  }
/* clang-format on */

typedef Value (*HashFunc)(Value *self);
typedef Value (*CmpFunc)(Value *lhs, Value *rhs);
typedef Value (*GetIterFunc)(Value *self);
typedef Value (*IterNextFunc)(Value *self);
typedef Value (*StrFunc)(Value *self);
typedef Value (*InitFunc)(Value *self, Value *args, int nargs);
typedef void (*FiniFunc)(Value *self);
typedef Value (*CallFunc)(Object *obj, Value *args, int nargs);
typedef Value (*CFunc)(Value *, Value *, int);

typedef struct _MethodDef {
    /* The name of function/method */
    char *name;
    /* The c func */
    CFunc cfunc;
    /* The arguments types */
    char *args_desc;
    /* The return type */
    char *ret_desc;
} MethodDef;

typedef Value (*GetterFunc)(Value *);
typedef int (*SetterFunc)(Value *, Value *);

typedef struct _GetSetDef {
    /* The name of the attribute */
    const char *name;
    /* The attribute getter function */
    GetterFunc get;
    /* The attribute setter function */
    SetterFunc set;
} GetSetDef;

#define TP_FLAGS_CLASS    (1 << 0)
#define TP_FLAGS_TRAIT    (1 << 1)
#define TP_FLAGS_ABSTRACT (1 << 2)
#define TP_FLAGS_PUBLIC   (1 << 3)
#define TP_FLAGS_FINAL    (1 << 4)
#define TP_FLAGS_HEAP     (1 << 5)

typedef struct _TypeObject {
    OBJECT_HEAD
    /* type name */
    char *name;
    /* one of TP_FLAGS_XXX */
    int flags;
    /* object size */
    size_t size;

    /* object call defaults */
    int kwds_offset;

    /* gc mark function */
    GcMarkFunc mark;

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
    /* getset definitions */
    GetSetDef *getsets;

    /* base class and traits */
    struct _TypeObject *base;
    struct _TraitObject **traits;

    /* attributes(fields, getsets) */
    Vector *attrs;

    /* functions(parent and self methods) */
    Vector *funcs;

    /* dynamic functions table */
    HashMap *vtbl;

} TypeObject;

extern TypeObject type_type;
TypeObject *value_type(Value *val);

static inline CallFunc object_is_callable(Object *obj)
{
    return OB_TYPE(obj)->call;
}

static inline CallFunc value_is_callable(Value *val)
{
    TypeObject *tp = value_type(val);
    return tp->call;
}

Value object_call(Object *obj, Value *args, int nargs);
Object *get_default_kwargs(Value *val);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
