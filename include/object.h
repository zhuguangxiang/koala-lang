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
    union {
        int64_t ival;
        double fval;
        Object *obj;
    };
    int tag;
} Value;

#define VAL_TAG_OBJ  (1 << 0)
#define VAL_TAG_NONE (1 << 1)
#define VAL_TAG_ERR  (1 << 2)
#define VAL_TAG_BOOL (1 << 3)
#define VAL_TAG_INT  (1 << 4)
#define VAL_TAG_FLT  (1 << 5)

#define IS_OBJECT(x) ((x)->tag & VAL_TAG_OBJ)
#define IS_NONE(x)   ((x)->tag & VAL_TAG_NONE)
#define IS_ERROR(x)  ((x)->tag & VAL_TAG_ERR)
#define IS_BOOL(x)   ((x)->tag & VAL_TAG_BOOL)
#define IS_INT(x)    ((x)->tag & VAL_TAG_INT)
#define IS_FLOAT(x)  ((x)->tag & VAL_TAG_FLT)

/* clang-format off */
#define ObjectValue(x)  (Value){ .obj = (x),  .tag = VAL_TAG_OBJ  }
#define NoneValue()     (Value){ .ival = 0,   .tag = VAL_TAG_NONE }
#define ErrorValue()    (Value){ .ival = 0,   .tag = VAL_TAG_ERR  }
#define TrueValue       (Value){ .ival = 1,   .tag = VAL_TAG_BOOL }
#define FalseValue      (Value){ .ival = 0,   .tag = VAL_TAG_BOOL }
#define IntValue(x)     (Value){ .ival = (x), .tag = VAL_TAG_INT  }
#define FloatValue(x)   (Value){ .fval = (x), .tag = VAL_TAG_FLT  }
/* clang-format on */

typedef unsigned int (*HashFunc)(Value *self);
typedef int (*CmpFunc)(Value *lhs, Value *rhs);
typedef Value (*GetIterFunc)(Value *self);
typedef Value (*IterNextFunc)(Value *self);
typedef Value (*StrFunc)(Value *self);
typedef Value (*CallFunc)(Value *self, Value *args, int nargs, Object *kwargs);
typedef Value (*UnaryFunc)(Value *);
typedef Value (*BinaryFunc)(Value *, Value *);
typedef Value (*TernaryFunc)(Value *, Value *, Value *);
typedef int (*LenFunc)(Value *);
typedef Value (*GetItemFunc)(Value *, Value *);
typedef int (*SetItemFunc)(Value *, Value *, Value *);
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
    GetItemFunc slice;
    SetItemFunc set_slice;
} SequenceMethods;

typedef struct _CFuncDef {
    /* The name of the built-in function/method */
    const char *name;
    /* The C function that implements it */
    void *cfunc;
    /* Combination of METH_xxx flags */
    int flags;
    /* keywords arguments(dict) */
    Object *kwargs;
    /* The koala arguments types */
    const char *args_types;
    /* The koala return type */
    const char *ret_type;
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

    /* object with default kwargs */
    int offset_kwargs;

    /* gc mark function */
    GcMarkFunc mark;

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

static inline Object *get_default_kwargs(Object *obj)
{
    TypeObject *tp = OB_TYPE(obj);
    if (tp->offset_kwargs) {
        return (Object *)((char *)obj + tp->offset_kwargs);
    } else {
        return NULL;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
