/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "util/common.h"
#include "util/hashmap.h"
#include "util/vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TypeInfo TypeInfo;
typedef struct _Object Object;

/* The macro indicates which is an `Object` */
#define OBJECT_HEAD TypeInfo *type;

/* The macro indicates an `Object` with type parameters */
#define GENERIC_OBJECT_HEAD OBJECT_HEAD uint32 tp_map;

struct _Object {
    OBJECT_HEAD
};

#define TP_REF  1
#define TP_I8   2
#define TP_I16  3
#define TP_I32  4
#define TP_I64  5
#define TP_F16  6
#define TP_F32  7
#define TP_F64  8
#define TP_BOOL 9
#define TP_CHAR 10
#define TP_MASK 15

#define tp_index(tp, idx) (((tp) >> ((idx)*4)) & TP_MASK)

#define is_ref(tp, idx)  (tp_index(tp, idx) == TP_REF)
#define is_i8(tp, idx)   (tp_index(tp, idx) == TP_I8)
#define is_i16(tp, idx)  (tp_index(tp, idx) == TP_I16)
#define is_i32(tp, idx)  (tp_index(tp, idx) == TP_I32)
#define is_i64(tp, idx)  (tp_index(tp, idx) == TP_I64)
#define is_f16(tp, idx)  (tp_index(tp, idx) == TP_F16)
#define is_f32(tp, idx)  (tp_index(tp, idx) == TP_F32)
#define is_f64(tp, idx)  (tp_index(tp, idx) == TP_F64)
#define is_bool(tp, idx) (tp_index(tp, idx) == TP_BOOL)
#define is_char(tp, idx) (tp_index(tp, idx) == TP_CHAR)

// clang-format off
#define TP_0(t0) (t0)
#define TP_1(t0, t1) (TP_1(t0) + (t1) << 4)
#define TP_2(t0, t1, t2) (TP_2(t0, t1) + (t2) << 8)
#define TP_3(t0, t1, t2, t3) (TP_3(t0, t1, t2) + (t3) << 12)
#define TP_4(t0, t1, t2, t3, t4) (TP_4(t0, t1, t2, t3) + (t4) << 16)
#define TP_5(t0, t1, t2, t3, t4, t5) (TP_5(t0, t1, t2, t3, t4) + (t5) << 20)
#define TP_6(t0, t1, t2, t3, t4, t5, t6) \
    (TP_6(t0, t1, t2, t3, t4, t5) + (t6) << 24)
#define TP_7(t0, t1, t2, t3, t4, t5, t6, t7) \
    (TP_7(t0, t1, t2, t3, t4, t5, t6) + (t7) << 28)
// clang-format on

int tp_size(uint32 tp_map, int index);

int32 generic_any_hash(uintptr obj, int isref);
int8 generic_any_equal(uintptr obj, uintptr other, int isref);

/* type flags */
#define TF_CLASS 1
#define TF_TRAIT 2
#define TF_ENUM  3
#define TF_MOD   4
#define TF_PUB   0x10
#define TF_FINAL 0x20

#define kl_is_class(type) (((type)->flags & 0x0F) == TF_CLASS)
#define kl_is_trait(type) (((type)->flags & 0x0F) == TF_TRAIT)
#define kl_is_enum(type)  (((type)->flags & 0x0F) == TF_ENUM)
#define kl_is_mod(type)   (((type)->flags & 0x0F) == TF_MOD)
#define kl_is_pub(type)   ((type)->flags & TF_PUB)
#define kl_is_final(type) ((type)->flags & TF_FINAL)

/* `Type` layout */
struct _TypeInfo {
    /* virtual table */
    uintptr **vtbl;
    /* object map */
    int *objmap;
    /* type name */
    char *name;
    /* type flags */
    int flags;
    /* type parameters */
    Vector *params;
    /* self methods */
    Vector *methods;
    /* self fields */
    Vector *fields;
    /* first class or trait */
    TypeInfo *base;
    /* other traits */
    Vector *traits;
    /* line resolution order */
    Vector *lro;
    /* all symbols */
    HashMap *mtbl;
    /* for singleton */
    Object *instance;
};

typedef struct _TypeParam TypeParam;

/* type parameter */
struct _TypeParam {
    char name[8];
    TypeInfo *bound;
};

/* create new klass type */
TypeInfo *kl_type_new(char *path, char *name, int flags, Vector *params,
                      TypeInfo *base, Vector *traits);

#define kl_type_new_simple(path, name, flags) \
    kl_type_new(path, name, flags, NULL, NULL, NULL)

/* be ready for this klass */
void kl_type_ready(TypeInfo *type);

/* add method of this klass */
void kl_add_method(TypeInfo *type, Object *meth);

/* add field of this klass */
void kl_add_field(TypeInfo *type, Object *field);

/* initialize types */
void kl_init_types(void);

/* finalize types */
void kl_fini_types(void);

/* show type */
void kl_type_show(TypeInfo *type);

typedef enum { KFUNC_KIND, CFUNC_KIND, PROTO_KIND } MethodKind;

/* 'Method' object layout */
typedef struct _MethodObject MethodObject;

struct _MethodObject {
    OBJECT_HEAD
    /* method name */
    char *name;
    /* parent type of this method */
    TypeInfo *parent;
    /* type desc */
    /* method kind */
    MethodKind kind;
    /* cfunc or kfunc */
    void *ptr;
    /* hashmap entry */
    HashMapEntry entry;
};

/* Methoddef struct */
typedef struct _MethodDef {
    char *name;
    char *ptype;
    char *rtype;
    char *fname;
    void *faddr;
} MethodDef;

// clang-format off

#define METHOD_DEF(name, ptype, rtype, func) \
    { name, ptype, rtype, #func, func }

// clang-format on

void kl_add_methoddefs(TypeInfo *type, MethodDef *def);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
