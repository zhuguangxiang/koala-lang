/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "typedesc.h"
#include "hashmap.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct object Object;
typedef struct typeobject TypeObject;

typedef Object *(*cfunc)(Object *, Object *);
typedef Object *(*getfunc)(Object *, char *name);

typedef struct {
  char *name;
  char *ptype;
  char *rtype;
  cfunc func;
} MethodDef;

typedef struct {
  char *name;
  char *type;
  cfunc getfunc;
  cfunc setfunc;
} FieldDef;

struct mnode {
  struct hashmap_entry entry;
  char *name;
  Object *obj;
};

struct mnode *mnode_new(char *name, Object *ob);
int mnode_compare(void *e1, void *e2);

/* object header */
#define OBJECT_HEAD      \
  /* reference count */  \
  int ob_refcnt;         \
  /* type structure */   \
  TypeObject *ob_type;

struct object {
  OBJECT_HEAD
};

#define OBJECT_HEAD_INIT(_type_) \
  .ob_refcnt = 1, .ob_type = (_type_),

#define Init_Object_Head(_ob_, _type_) \
({                                     \
  Object *ob = (Object *)(_ob_);       \
  ob->ob_refcnt = 1;                   \
  ob->ob_type = (_type_);              \
})

#define OB_TYPE(_ob_) \
  ((Object *)(_ob_))->ob_type

#define OB_INCREF(_ob_)         \
({                              \
  Object *o = (Object *)(_ob_); \
  if (o != NULL)                \
    o->ob_refcnt++;             \
  (_ob_);                       \
})

#define OB_DECREF(_ob_)         \
({                              \
  Object *o = (Object *)(_ob_); \
  if (o != NULL) {              \
    --o->ob_refcnt;             \
    panic(o->ob_refcnt < 0,     \
          "refcnt %d error",    \
          o->ob_refcnt);        \
    if (o->ob_refcnt <= 0) {    \
      OB_TYPE(o)->freefunc(o);  \
      (_ob_) = NULL;            \
    }                           \
  }                             \
})

typedef struct {
  /* arithmetic */
  cfunc add;
  cfunc sub;
  cfunc mul;
  cfunc div;
  cfunc mod;
  cfunc pow;
  cfunc neg;
  /* relational */
  cfunc gt;
  cfunc ge;
  cfunc lt;
  cfunc le;
  cfunc eq;
  cfunc neq;
  /* bit */
  cfunc band;
  cfunc bor;
  cfunc bxor;
  cfunc bnot;
  /* logic */
  cfunc land;
  cfunc lor;
  cfunc lnot;
} NumberMethods;

/*
 * map operations for array and map,
 * If objects override '[]' operator, they also have map operations.
 */
typedef struct {
  cfunc getitem;
  cfunc setitem;
} MappingMethods;

/* mark and sweep callback */
typedef void (*markfunc)(Object *);

/* alloc object function */
typedef Object *(*allocfunc)(TypeObject *);

/* free object function */
typedef void (*freefunc)(Object *);

#define TPFLAGS_CLASS 1
#define TPFLAGS_TRAIT 2
#define TPFLAGS_ENUM  3
#define TPFLAGS_GC    4

/* klass structure */
struct typeobject {
  OBJECT_HEAD
  /* type name */
  char *name;
  /* one of KLASS_xxx */
  int flags;

  /* mark-and-sweep */
  markfunc markfunc;
  /* alloc and free */
  allocfunc allocfunc;
  freefunc freefunc;
  /* get meta member */
  getfunc getfunc;

  /* hash_code() */
  cfunc hashfunc;
  /* equal() */
  cfunc equalfunc;
  /* get_class() */
  cfunc classfunc;
  /* tostr() */
  cfunc strfunc;

  /* number override operators */
  NumberMethods *number;
  /* subscript operations */
  MappingMethods *mapping;

  /* tuple: base classes */
  struct vector *bases;
  /* line resolution order */
  struct vector *lro;
  /* map: meta table */
  struct hashmap *mtbl;

  /* fields definition */
  FieldDef *fields;
  /* methods definition */
  MethodDef *methods;

  /* owner module */
  Object *module;
  /* offset of fields */
  int offset;
  /* number of fields */
  int nrvars;
  /* constant pool */
  Object *consts;
};

extern TypeObject Type_Type;
extern TypeObject Any_Type;
static inline
int Type_Equal(TypeObject *type1, TypeObject *type2)
{
  return type1 == type2;
}
int Type_Ready(TypeObject *type);
int Type_Add_Field(TypeObject *type, Object *ob);
int Type_Add_Method(TypeObject *type, Object *ob);
int Type_Add_CFunc(TypeObject *type, MethodDef *func);

Object *_object_member_(Object *self, char *name);
Object *_object_class_(Object *ob, Object *args);
Object *Object_CallMethod(Object *self, char *name, Object *args);
Object *Object_GetValue(Object *self, char *name);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
