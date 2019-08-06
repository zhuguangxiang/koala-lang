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

typedef Object *(*func_t)(Object *, Object *);
typedef Object *(*lookupfunc)(Object *, char *name);
typedef Object *(*callfunc)(Object *, Object *, Object *);
typedef int (*setfunc)(Object *, Object *, Object *);

typedef struct {
  char *name;
  char *ptype;
  char *rtype;
  func_t func;
} MethodDef;

typedef struct {
  char *name;
  char *type;
  func_t get;
  setfunc set;
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

#define OB_TYPE_NAME(_ob_) \
  (OB_TYPE(_ob_)->name)

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
      OB_TYPE(o)->free(o);      \
      (_ob_) = NULL;            \
    }                           \
  }                             \
})

typedef struct {
  /* arithmetic */
  func_t add;
  func_t sub;
  func_t mul;
  func_t div;
  func_t mod;
  func_t pow;
  func_t neg;
  /* relational */
  func_t gt;
  func_t ge;
  func_t lt;
  func_t le;
  func_t eq;
  func_t neq;
  /* bit */
  func_t band;
  func_t bor;
  func_t bxor;
  func_t bnot;
  /* logic */
  func_t land;
  func_t lor;
  func_t lnot;
} NumberMethods;

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
  /* type's module */
  Object *owner;

  /* mark-and-sweep */
  markfunc mark;
  /* alloc and free */
  allocfunc alloc;
  freefunc free;

  /* hash_code() */
  func_t hash;
  /* equal() */
  func_t equal;
  /* get_class() */
  func_t clazz;
  /* tostr() */
  func_t str;
  /* fmt() */
  func_t fmt;

  /* number operations */
  NumberMethods *number;
  /* sequence operations */
//  MappingMethods *mapping;
  /* iterator operations */
//  IteratorMethods *iterator;

  /* tuple: base classes */
  struct vector *bases;
  /* line resolution order */
  struct vector *lro;
  /* map: meta table */
  struct hashmap *mtbl;

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
#define Type_Check(ob) (OB_TYPE(ob) == &Type_Type)
int Type_Ready(TypeObject *type);
Object *Type_Lookup(TypeObject *type, char *name);

void Type_Add_Field(TypeObject *type, Object *ob);
void Type_Add_FieldDef(TypeObject *type, FieldDef *f);
void Type_Add_FieldDefs(TypeObject *type, FieldDef *def);

void Type_Add_Method(TypeObject *type, Object *ob);
void Type_Add_MethodDef(TypeObject *type, MethodDef *f);
void Type_Add_MethodDefs(TypeObject *type, MethodDef *def);

unsigned int Object_Hash(Object *ob);
int Object_Equal(Object *ob1, Object *ob2);

Object *Object_GetValue(Object *self, char *name);
int Object_SetValue(Object *self, char *name, Object *val);
Object *Object_Call(Object *self, char *name, Object *args);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
