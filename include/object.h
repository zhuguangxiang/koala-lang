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
    /* value */
    union {
        int64_t ival;
        double fval;
        Object *obj;
    };
    /* tag */
    uintptr_t tag : 4;
} Value;

/*
    +-------+----------------+
    |  tag  |   value        |
    +-------+----------------+
    |  0000 |  int32         |
    +-------+----------------+
    |  0010 |  int64         |
    +-------+----------------+
    |  0100 |  float32       |
    +-------+----------------+
    |  0110 |  float54       |
    +-------+----------------+
    |  1010 |  Error         |
    +-------+----------------+
    |  1110 |  None          |
    +-------+----------------+
    |  1111 |  obj(gc)       |
    +-------+----------------+
*/
#define VAL_TAG_I32  0b0000
#define VAL_TAG_I64  0b0010
#define VAL_TAG_F32  0b0100
#define VAL_TAG_F64  0b0110
#define VAL_TAG_ERR  0b1010
#define VAL_TAG_NONE 0b1110
#define VAL_TAG_OBJ  0b1111

/* clang-format off */

#define IS_INT32(val) ((val)->tag == VAL_TAG_I32)
#define IS_INT64(val) ((val)->tag == VAL_TAG_I64)
#define IS_INT(val) (IS_INT32(val) || IS_INT64(val))

#define IS_FLOAT32(val) ((val)->tag == VAL_TAG_F32)
#define IS_FLOAT64(val) ((val)->tag == VAL_TAG_F64)
#define IS_FLOAT(val) (IS_FLOAT32(val) || IS_FLOAT64(val))

#define IS_ERROR(val) ((val)->tag == VAL_TAG_ERR)
#define IS_NONE(val) ((val)->tag == VAL_TAG_NONE)

#define IS_OBJECT(val) ((val)->tag == VAL_TAG_OBJ)

#define Int32Value()
#define ObjectValue(_obj) (Value){.obj = _obj, .tag = VAL_TAG_OBJ}
#define NoneValue() (Value){0, VAL_TAG_NONE}
#define ErrorValue() (Value){0, VAL_TAG_ERR}

/* clang-format on */

typedef unsigned int (*HashFunc)(Value *self);
typedef int (*CmpFunc)(Value *lhs, Value *rhs);
typedef Value (*GetIterFunc)(Value *self);
typedef Value (*IterNextFunc)(Value *self);
typedef Value (*StrFunc)(Value *self);
typedef Value (*CallFunc)(Value *self, Value *args, int nargs, Object *kwargs);
typedef int (*LenFunc)(Value *);
typedef Value (*UnaryFunc)(Value *);
typedef Value (*BinaryFunc)(Value *, Value *);
typedef Value (*TernaryFunc)(Value *, Value *, Value *);

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
    BinaryFunc ushr;
    /* clang-format on */
} NumberMethods;

typedef struct _SequenceMethods {
    LenFunc len;
    BinaryFunc concat;
    BinaryFunc getitem;
    TernaryFunc setitem;
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

/* clang-format off */
#define KLASS_OBJECT_HEAD OBJECT_HEAD char *name; int flags;
/* clang-format on */

typedef struct _KlassObject {
    KLASS_OBJECT_HEAD
} KlassObject;

typedef struct _TypeObject {
    KLASS_OBJECT_HEAD

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

typedef struct _TraitObject {
    KLASS_OBJECT_HEAD
    /* abstract method definitions */
    MethodDef *methods;
    /* parent traits */
    struct _TraitObject **traits;
} TraitObject;

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
