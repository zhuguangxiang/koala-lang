/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
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
    char *name;
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
    /* next offset */
    int next_offset;
    /* for singleton */
    Object *instance;
};

#define obj_get_type(obj)       *((void **)(obj)-1)
#define obj_set_type(obj, type) *((void **)(obj)-1) = (type)

extern TypeObject *any_type;
extern TypeObject *type_type;
#define type_check(obj) (obj_get_type(obj) == type_type)

#define type_is_class(type) (((type)->flags & 0x0F) == TP_FLAGS_CLASS)
#define type_is_trait(type) (((type)->flags & 0x0F) == TP_FLAGS_TRAIT)
#define type_is_enum(type)  (((type)->flags & 0x0F) == TP_FLAGS_ENUM)
#define type_is_mod(type)   (((type)->flags & 0x0F) == TP_FLAGS_MOD)
#define type_is_pub(type)   (((type)->flags & TP_FLAGS_PUB) == TP_FLAGS_PUB)
#define type_is_final(type) (((type)->flags & TP_FLAGS_FINAL) == TP_FLAGS_FINAL)

#define type_set_singleton(type, obj) (type)->instance = (Object *)obj

/* allocate meta object */
void *__alloc_meta_object(int size);
#define alloc_meta_object(type) (type *)__alloc_meta_object(sizeof(type))

static inline TypeObject *__type_new(char *name)
{
    TypeObject *type = alloc_meta_object(TypeObject);
    type->name = name;
    return type;
}

/* new class */
static inline TypeObject *type_new_class(char *name)
{
    TypeObject *type = __type_new(name);
    type->flags = TP_FLAGS_CLASS;
    return type;
}

/* new trait */
static inline TypeObject *type_new_trait(char *name)
{
    TypeObject *type = __type_new(name);
    type->flags = TP_FLAGS_TRAIT;
    return type;
}

static inline TypeObject *type_new_pub_trait(char *name)
{
    TypeObject *type = __type_new(name);
    type->flags = TP_FLAGS_TRAIT + TP_FLAGS_PUB;
    return type;
}

/* new enum */
static inline TypeObject *type_new_enum(char *name)
{
    TypeObject *type = __type_new(name);
    type->flags = TP_FLAGS_ENUM;
    return type;
}

/* new mod */
static inline TypeObject *type_new_mod(char *name)
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

/* create a meta table */
HashMap *mtbl_create(void);

/* add new object to meta table */
int mtbl_add(HashMap *mtbl, char *name, void *obj);

/* find an object from a meta table */
void *mtbl_find(HashMap *mtbl, char *name);

/* destroy a meta table */
void mtbl_destroy(HashMap *mtbl);

/* show meta table */
void mtbl_show(HashMap *mtbl);

/* visit meta table */
typedef void (*mvisit_t)(char *name, Object *obj, void *arg);
void mtbl_visit(HashMap *mtbl, mvisit_t func, void *arg);

/* method define struct */
typedef struct MethodDef {
    char *name;
    char *ptype;
    char *rtype;
    char *funcname;
} MethodDef;

/* field define struct */
typedef struct FieldDef {
    char *name;
    char *type;
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

/* `Field` object layout */
typedef struct FieldObject {
    OBJECT_HEAD
    /* field name */
    char *name;
    /* field type desc */
    TypeDesc *desc;
    /* offset */
    int offset;
    /* size */
    int size;
    /* field pointer */
    void *ptr;
} FieldObject;

extern TypeObject *field_type;
#define field_check(obj) (obj_get_type(obj) == field_type)

/* new field */
Object *field_new(char *name);

/* method kind */
typedef enum {
    KFUNC_KIND = 1,
    CFUNC_KIND,
    PROTO_KIND,
} meth_kind_t;

/* `Method` object layout */
typedef struct MethodObject {
    OBJECT_HEAD
    /* method name */
    char *name;
    /* method type desc */
    TypeDesc *desc;
    /* method kind */
    meth_kind_t kind;
    /* cfunc or kcode */
    void *ptr;
} MethodObject;

extern TypeObject *method_type;
#define method_check(obj) (obj_get_type(obj) == method_type)

/* new c method */
Object *cmethod_new(MethodDef *def);

/* new koala method */
Object *method_new(char *name, Object *code);

/* get method number of locals(kcode only) */
int method_get_nloc(Object *meth);

/* type(module) add type */
int type_add_type(TypeObject *mod, TypeObject *type);

/* type add field */
int type_add_field(TypeObject *mod, FieldObject *type);

/* type add method */
int type_add_method(TypeObject *mod, MethodObject *type);

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
