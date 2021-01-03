/*===-- object.c - Koala Object Model -----------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares the Koala `Object` and `TypeObject` object.           *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "common.h"
#include "gc.h"
#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------ core data types ------------*/

/*
 * The common fields are hidden before the pointer, and the `OBJECT_HEAD`
 * macro is used to indicate which is an `Object`.
 * see: `GcObject` in gc.h
 */
#define OBJECT_HEAD

/* The `Object` type is an opaque type, it likes `void` in c. */
typedef struct Object Object;

/*
 * The `TypeObject` type is koala meta type, represents `class`, `trait`,
 * `enum` and `module`.
 */
typedef struct TypeObject TypeObject;

/* type's flags */
typedef enum {
    TP_FLAGS_CLASS = 1,
    TP_FLAGS_TRAIT,
    TP_FLAGS_ENUM,
    TP_FLAGS_MOD,
    TP_FLAGS_PUB = 0x10,
    TP_FLAGS_FINAL = 0x20,
} tp_flags_t;

/* `Type` object layout */
struct TypeObject {
    /* object */
    OBJECT_HEAD
    /* virtual table */
    void *vtbl;
    /* gc object map */
    objmap_t *objmap;
    /* type name */
    const char *name;
    /* type flags */
    tp_flags_t flags;
    /* first super type(class or trait) */
    TypeObject *base;
    /* other super types(traits) */
    Vector *traits;
    /* self methods */
    Vector *methods;
    /* line resolution order */
    Vector *lro;
    /* meta table */
    HashMap *mtbl;
    /* lro methods */
    Vector *mro;
};

#define obj_get_type(obj)       *((void **)(obj)-1)
#define obj_set_type(obj, type) *((void **)(obj)-1) = (type)

#define type_is_class(type) (((type)->flags & 0x0F) == TP_FLAGS_CLASS)
#define type_is_trait(type) (((type)->flags & 0x0F) == TP_FLAGS_TRAIT)
#define type_is_enum(type)  (((type)->flags & 0x0F) == TP_FLAGS_ENUM)
#define type_is_mod(type)   (((type)->flags & 0x0F) == TP_FLAGS_MOD)
#define type_is_pub(type)   (((type)->flags & TP_FLAGS_PUB) == TP_FLAGS_PUB)
#define type_is_final(type) (((type)->flags & TP_FLAGS_FINAL) == TP_FLAGS_FINAL)

/* allocate meta object */
void *__alloc_meta_object(int size);
#define alloc_meta_object(type) (type *)__alloc_meta_object(sizeof(type))

static inline TypeObject *__type_new(const char *name)
{
    TypeObject *type = alloc_meta_object(TypeObject);
    type->name = name;
    return type;
}

/* new class */
static inline TypeObject *type_new_class(const char *name)
{
    TypeObject *type = __type_new(name);
    type->flags = TP_FLAGS_CLASS;
    return type;
}

/* new trait */
static inline TypeObject *type_new_trait(const char *name)
{
    TypeObject *type = __type_new(name);
    type->flags = TP_FLAGS_TRAIT;
    return type;
}

static inline TypeObject *type_new_pub_trait(const char *name)
{
    TypeObject *type = __type_new(name);
    type->flags = TP_FLAGS_TRAIT + TP_FLAGS_PUB;
    return type;
}

/* new enum */
static inline TypeObject *type_new_enum(const char *name)
{
    TypeObject *type = __type_new(name);
    type->flags = TP_FLAGS_ENUM;
    return type;
}

/* new mod */
static inline TypeObject *type_new_mod(const char *name)
{
    TypeObject *type = __type_new(name);
    type->flags = TP_FLAGS_MOD;
    return type;
}

/* set type's public */
#define type_set_public(type) (type)->flags |= TP_FLAGS_PUB

/* set type's final */
#define type_set_final(type) (type)->flags |= TP_FLAGS_FINAL

/* set type's public final */
#define type_set_public_final(type) \
    (type)->flags |= (TP_FLAGS_PUB + TP_FLAGS_FINAL)

/* type_ready called must be at last */
void type_ready(TypeObject *type);

/* append base class or traits */
void type_append_base(TypeObject *type, TypeObject *base);

/* show type */
void type_show(TypeObject *type);

/* initialize core types(Any and Type) */
void init_core_types(void);

/* method define struct */
typedef struct MethodDef {
    const char *name;
    const char *ptype;
    const char *rtype;
    const char *funcname;
} MethodDef;

/* field define struct */
typedef struct FieldDef {
    const char *name;
    const char *type;
} FieldDef;

/* add method to type */
int type_add_methdef(TypeObject *type, MethodDef *def);

/* add field to type */
int type_add_fielddef(TypeObject *type, FieldDef *def);

/* add methods to type */
static inline void type_add_methdefs(TypeObject *type, MethodDef *def)
{
    MethodDef *meth = def;
    while (meth->name) {
        type_add_methdef(type, meth);
        meth++;
    }
}

/* add fields to type */
static inline void type_add_fielddefs(TypeObject *type, FieldDef *def)
{
    FieldDef *field = def;
    while (field->name) {
        type_add_fielddef(type, field);
        field++;
    }
}

/* Type and fat pointer layout */
typedef union {
    struct {
        char tag : 2;
        char kind;
    };
    void *vtbl;
} TypeRef;

/* Value layout */
typedef union {
    /* gc object */
    Object *obj;
    /* c struct */
    void *ptr;
    /* integer */
    int64_t ival;
    /* float */
    double fval;
    /* byte */
    int8_t bval;
    /* bool */
    int8_t zval;
    /* character */
    int32_t cval;
} ValueRef;

/*
 * `TValueRef` is variables layout in vm stack only.
 * The global variables and fields in object are as binary(offset) layout.
 */
typedef struct {
    TypeRef _t;
    ValueRef _v;
} TValueRef;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
