/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "typedesc.h"
#include "hashmap.h"
#include "vector.h"
#include "log.h"

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
  HashMapEntry entry;
  char *name;
  Object *obj;
};

struct mnode *mnode_new(char *name, Object *ob);
void mnode_free(void *e, void *arg);
int mnode_equal(void *e1, void *e2);

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

#define init_object_head(_ob_, _type_) \
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
    expect(o->ob_refcnt >= 0);  \
    if (o->ob_refcnt == 0) {    \
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
  func_t and;
  func_t or;
  func_t xor;
  func_t not;
} NumberMethods;

typedef struct {
  /* arithmetic */
  func_t add;
  func_t sub;
  func_t mul;
  func_t div;
  func_t mod;
  func_t pow;
  /* bit */
  func_t and;
  func_t or;
  func_t xor;
} InplaceMethods;

typedef struct {
  /* __getitem__ */
  func_t getitem;
  /* __setitem__ */
  func_t setitem;
  /* __getslice__ */
  func_t getslice;
  /* __setslice__ */
  func_t setslice;
} MappingMethods;

typedef struct {
  /* __iter__ */
  func_t iter;
  /* __next__ */
  func_t next;
} IteratorMethods;

/* mark and sweep callback */
typedef void (*ob_markfunc)(Object *);

/* alloc object function */
typedef Object *(*ob_allocfunc)(TypeObject *);

/* free object function */
typedef void (*ob_freefunc)(Object *);

#define TPFLAGS_CLASS     0
#define TPFLAGS_ABSTRACT  1
#define TPFLAGS_TRAIT     2
#define TPFLAGS_ENUM      3

struct typeobject {
  OBJECT_HEAD
  /* type name */
  char *name;
  /* one of TPFLAGS_xxx */
  int flags;
  /* type decriptor */
  TypeDesc *desc;

  /* mark-and-sweep */
  ob_markfunc mark;
  /* alloc and free */
  ob_allocfunc alloc;
  ob_freefunc free;

  /* __hash__  */
  func_t hash;
  /* __cmp__ */
  func_t cmp;
  /* __class__ */
  func_t clazz;
  /* __str__   */
  func_t str;

  /* number operations */
  NumberMethods *number;
  /* inplace number operations */
  InplaceMethods *inplace;
  /* mapping operations */
  MappingMethods *mapping;
  /* iterator operations */
  IteratorMethods *iterator;

  /* tuple: base classes */
  Vector *bases;
  /* line resolution order */
  Vector lro;
  /* map: meta table */
  HashMap *mtbl;

  /* methods definition */
  MethodDef *methods;

  /* in which module */
  Object *owner;
  /* offset of fields */
  int offset;
  /* number of fields */
  int nrvars;
  /* constant pool */
  Object *consts;
};

extern TypeObject type_type;
extern TypeObject any_type;
#define Type_Check(ob) (OB_TYPE(ob) == &type_type)
void init_any_type(void);
#define OB_NUM_FUNC(ob, name) ({ \
  NumberMethods *nu = OB_TYPE(ob)->number; \
  nu ? nu->name : NULL; \
})
#define OB_INPLACE_FUNC(ob, name) ({ \
  InplaceMethods *nu = OB_TYPE(ob)->inplace; \
  nu ? nu->name : NULL; \
})
int type_ready(TypeObject *type);
void type_fini(TypeObject *type);
Object *Type_Lookup(TypeObject *type, char *name);

void Type_Add_Field(TypeObject *type, Object *ob);
void Type_Add_FieldDef(TypeObject *type, FieldDef *f);
void Type_Add_FieldDefs(TypeObject *type, FieldDef *def);

void Type_Add_Method(TypeObject *type, Object *ob);
void Type_Add_MethodDef(TypeObject *type, MethodDef *f);
void Type_Add_MethodDefs(TypeObject *type, MethodDef *def);

unsigned int Object_Hash(Object *ob);
int Object_Cmp(Object *ob1, Object *ob2);
Object *Object_Lookup(Object *self, char *name);
Object *Object_GetMethod(Object *self, char *name);
Object *Object_GetField(Object *self, char *name);
Object *Object_GetValue(Object *self, char *name);
int Object_SetValue(Object *self, char *name, Object *val);
Object *Object_Call(Object *self, char *name, Object *args);
Object *New_Literal(Literal *val);

typedef struct descobject {
  OBJECT_HEAD
  TypeDesc *desc;
} DescObject;

extern TypeObject descob_type;
#define descob_check(ob) (OB_TYPE(ob) == &descob_type)
void init_descob_type(void);
Object *new_descob(TypeDesc *desc);
#define descob_getdesc(ob)    \
({                            \
  expect(descob_check(ob));   \
  ((DescObject *)(ob))->desc; \
})

typedef struct enumobject {
  OBJECT_HEAD
  char *name;
  Vector *values;
} EnumObject;

Object *new_eval(TypeObject *type, char *name, Vector *vals);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
