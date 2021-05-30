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

/* The macro is used to indicate which is an `Object` */
#define OBJECT_HEAD

/* The `Object` type is an opaque type, it likes `void` in c. */
typedef struct Object *ObjectRef;

/*
 * The `TypeObject` type is koala meta type, represents `class`, `trait`,
 * `enum` and `module`.
 */
typedef struct _TypeObject *TypeObjectRef;

/* type's flags */
typedef enum {
    TF_CLASS = 1,
    TF_TRAIT,
    TF_ENUM,
    TF_MOD,
    TF_PUB = 0x10,
    TF_FINAL = 0x20,
} TPFlags;

#define kl_is_class(type) (((type)->flags & 0x0F) == TF_CLASS)
#define kl_is_trait(type) (((type)->flags & 0x0F) == TF_TRAIT)
#define kl_is_enum(type)  (((type)->flags & 0x0F) == TF_ENUM)
#define kl_is_mod(type)   (((type)->flags & 0x0F) == TF_MOD)
#define kl_is_pub(type)   ((type)->flags & TF_PUB)
#define kl_is_final(type) ((type)->flags & TF_FINAL)

/* `Type` object layout */
struct _TypeObject {
    OBJECT_HEAD
    /* object map */
    // objmap_t *objmap;
    /* mark */
    /* virtual table */
    VectorRef vtbl;
    /* type name */
    char *name;
    /* type flags */
    TPFlags flags;
    /* type parameters */
    VectorRef params;
    /* self methods */
    VectorRef methods;
    /* self fields */
    VectorRef fields;
    /* first class or trait */
    TypeObjectRef base;
    /* other traits */
    VectorRef traits;
    /* line resolution order */
    VectorRef lro;
    /* all symbols(includes bases) */
    HashMapRef mtbl;
    /* for singleton */
    ObjectRef instance;
};

/* type parameter */
typedef struct _TypeParam {
    char name[8];
    VectorRef bounds;
} TypeParam, *TypeParamRef;

/* create new klass type */
TypeObjectRef kl_type_new(char *path, char *name, TPFlags flags,
                          VectorRef params, TypeObjectRef base,
                          VectorRef traits);

#define kl_type_new_simple(path, name, flags) \
    kl_type_new(path, name, flags, NULL, NULL, NULL)

/* be ready for this klass */
void kl_type_ready(TypeObjectRef type);

/* add method of this klass */
void kl_add_method(TypeObjectRef type, ObjectRef meth);

/* add field of this klass */
void kl_add_field(TypeObjectRef type, ObjectRef field);

/* initialize types */
void kl_init_types(void);

/* finalize types */
void kl_fini_types(void);

/* show type */
void kl_type_show(TypeObjectRef type);

/* variable(stack, field) layout */
typedef struct _Value {
    uintptr_t ty;
    uintptr_t val;
} Value, *ValueRef;

/* clang-format off */
#define VALUE_INIT(_ty, _val) \
    { .ty = (uintptr_t)(_ty), .val = (uintptr_t)(_val) }
/* clang-format on */

#define IS_PRIM(v)  (((v).ty & 1)
#define SET_PRIM(v) ((v).ty |= 1)
#define GET_TYPE(v) ((v).ty & ~1)
#define GET_VAL(v)  ((v).val)

typedef enum { KFUNC_KIND, CFUNC_KIND, PROTO_KIND } MethodKind;

/* 'Method' object layout */
typedef struct _MethodObject {
    OBJECT_HEAD
    /* method name */
    char *name;
    /* parent type of this method */
    TypeObjectRef parent;
    /* type desc */
    /* method kind */
    MethodKind kind;
    /* cfunc or kfunc */
    void *ptr;
    /* hashmap entry */
    HashMapEntry entry;
} MethodObject, *MethodObjectRef;

/* Methoddef struct */
typedef struct _MethodDef {
    char *name;
    char *ptype;
    char *rtype;
    void *func;
} MethodDef, *MethodDefRef;

void kl_add_methoddefs(TypeObjectRef type, MethodDefRef def);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
