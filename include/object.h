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
    int8_t tag;
    union {
        int64_t ival;
        double fval;
        Object *obj;
    };
} Value;

#define VAL_TAG_OBJ  (1 << 0)
#define VAL_TAG_ERR  (1 << 1)
#define VAL_TAG_NONE (1 << 2)
#define VAL_TAG_INT  (1 << 3)
#define VAL_TAG_FLT  (1 << 4)

#define IS_OBJECT(x) ((x)->tag & VAL_TAG_OBJ)
#define IS_ERROR(x)  ((x)->tag & VAL_TAG_ERR)
#define IS_NONE(x)   ((x)->tag & VAL_TAG_NONE)
#define IS_INT(x)    ((x)->tag & VAL_TAG_INT)
#define IS_FLOAT(x)  ((x)->tag & VAL_TAG_FLT)

/* clang-format off */
#define ObjectValue(x)  (Value){ .tag = VAL_TAG_OBJ, .obj = (x)  }
#define ErrorValue      (Value){ .tag = VAL_TAG_ERR  }
#define NoneValue       (Value){ .tag = VAL_TAG_NONE }
#define IntValue(x)     (Value){ .tag = VAL_TAG_INT, .ival = (x) }
#define FloatValue(x)   (Value){ .tag = VAL_TAG_FLT, .fval = (x) }
/* clang-format on */

typedef unsigned int (*HashFunc)(Value *self);
typedef Value (*CmpFunc)(Value *lhs, Value *rhs);
typedef Value (*GetIterFunc)(Value *self);
typedef Value (*IterNextFunc)(Value *self);
typedef Value (*StrFunc)(Value *self);
typedef Value (*CallFunc)(Object *self, Value *args, int nargs, Object *kwargs);
typedef Value (*UnaryFunc)(Value *);
typedef Value (*BinaryFunc)(Value *, Value *);
typedef Value (*TernaryFunc)(Value *, Value *, Value *);
typedef int (*LenFunc)(Value *);
typedef Value (*GetItemFunc)(Value *, ssize_t);
typedef int (*SetItemFunc)(Value *, ssize_t, Value *);
typedef Value (*GetSliceFunc)(Value *, Object *);
typedef int (*SetSliceFunc)(Value *, Object *, Value *);
typedef int (*ContainsFunc)(Value *, Value *);

/* number protocol methods */
typedef struct _NumberMethods {
    BinaryFunc add;
    BinaryFunc sub;
    BinaryFunc mul;
    BinaryFunc div;
    BinaryFunc mod;
    UnaryFunc neg;
    /* clang-format off */
    BinaryFunc and;
    BinaryFunc or;
    BinaryFunc xor;
    UnaryFunc not;
    BinaryFunc shl;
    BinaryFunc shr;
    /* clang-format on */
} NumberMethods;

/* sequence protocol methods */
typedef struct _SequenceMethods {
    LenFunc len;
    BinaryFunc concat;
    ContainsFunc contains;
    GetItemFunc item;
    SetItemFunc set_item;
    GetSliceFunc slice;
    SetSliceFunc set_slice;
} SequenceMethods;

typedef struct _MethodDef {
    /* The name of function/method */
    char *name;
    /* The c func */
    void *cfunc;
    /* Combination of METH_xxx flags */
    int flags;
    /* The arguments types */
    char *args_desc;
    /* The return type */
    char *ret_desc;
} MethodDef;

typedef Value (*GetterFunc)(Value *, void *);
typedef int (*SetterFunc)(Value *, Value *, void *);

typedef struct _GetSetDef {
    /* The name of the attribute */
    const char *name;
    /* The attribute getter function */
    GetterFunc get;
    /* The attribute setter function */
    SetterFunc set;
    /* private closure */
    void *closure;
} GetSetDef;

/* Types for MemberDef */
#define MT_INT32   1
#define MT_INT64   2
#define MT_FLOAT32 3
#define MT_FLOAT64 4
#define MT_OBJECT  5

typedef struct _MemberDef {
    /* The name of the member */
    const char *name;
    /* The type of the member */
    int type;
    /* The offset of c struct */
    int offset;
} MemberDef;

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

    /* object call defaults */
    int kwargs_offset;

    /* gc mark function */
    GcMarkFunc mark;

    /* new function */
    /* init function */
    /* fini function */

    /* enter function */
    /* exit function */

    /* hashable */
    HashFunc hash;
    CmpFunc cmp;

    /* printable */
    StrFunc str;

    /* callable */
    CallFunc call;

    /* call with kwargs */
    Object *kwargs;

    /* iterable */
    GetIterFunc iter;
    IterNextFunc next;

    /* number protocol */
    NumberMethods *as_num;
    /* sequence protocol */
    SequenceMethods *as_seq;

    /* method definitions */
    MethodDef *methods;
    /* getset definitions */
    GetSetDef *getsets;
    /* member definitions */
    MemberDef *members;

    /* base class and traits */
    struct _TypeObject *base;
    struct _TraitObject **traits;

    /* attributes(fields, getsets and members) */
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

Value object_call(Object *callable, Value *args, int nargs, Object *kwargs);
Object *get_default_kwargs(Value *val);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
