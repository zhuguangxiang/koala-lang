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

#include "hashmap.h"
#include "vector.h"
#include <stdint.h>

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

/* meta table */
typedef struct kl_mtbl_t kl_mtbl_t;

/* create meta table */
kl_mtbl_t *mtbl_create(void);

/* add object into meta table, return -1 means failed. */
int mtbl_add(kl_mtbl_t *mtbl, const char *name, Object *obj);

/* remove object from meta table, return removed object. */
Object *mtbl_remove(kl_mtbl_t *mtbl, const char *name);

/* destroy meta table */
void mtbl_destroy(kl_mtbl_t *mtbl);

typedef enum {
    TP_FLAGS_CLASS = 1,
    TP_FLAGS_TRAIT,
    TP_FLAGS_ENUM,
    TP_FLAGS_MOD,
    TP_FLAGS_PUB = 0x10,
    TP_FLAGS_FINAL = 0x20,
} tp_flags_t;

struct TypeObject {
    /* object */
    OBJECT_HEAD
    /* type name */
    const char *name;
    /* type flags */
    tp_flags_t flags;
    /* super class or traits */
    Vector *bases;
    /* line resolution order */
    Vector lro;
    /* meta table */
    kl_mtbl_t *mtbl;
};

#define is_class(type) (((type)->flags & 0x0F) == TP_FLAGS_CLASS)
#define is_trait(type) (((type)->flags & 0x0F) == TP_FLAGS_TRAIT)
#define is_enum(type)  (((type)->flags & 0x0F) == TP_FLAGS_ENUM)
#define is_mod(type)   (((type)->flags & 0x0F) == TP_FLAGS_MOD)
#define is_pub(type)   (((type)->flags & TP_FLAGS_PUB) == TP_FLAGS_PUB)
#define is_final(type) (((type)->flags & TP_FLAGS_FINAL) == TP_FLAGS_FINAL)

/* new typeobject */
TypeObject *type_new(const char *name);

/* type_ready called must be at last */
void type_ready(TypeObject *type);

/* initialize `Any` and Type` */
void init_type_type(void);

/* method define struct */
typedef struct MethodDef {
    const char *name;
    const char *rtype;
    const char *ptype;
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

/* `kl_value_t` stores variable or field value. */
typedef struct kl_value {
    union {
        int64_t ival;
        double fval;
        int8_t bval;
        int32_t cval;
        void *ptr;
    };
    char kind;
} kl_value_t;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
