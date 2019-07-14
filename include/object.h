/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "hashmap.h"
#include "vector.h"
#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct typeobject TypeObject;
typedef struct object Object;

#define MBR_CONST 0
#define MBR_VAR   1
#define MBR_FUNC  2
#define MBR_PROTO 3
#define MBR_CLASS 4
#define MBR_ENUM  5
#define MBR_EVAL  6
#define MBR_MAX   7

/* member structure */
struct member {
  /* hashmap entry */
  struct hashmap_entry entry;
  /* member name */
  char *name;
  /* kind of member */
  int kind;
  /* its typedesc */
  TypeDesc *desc;
  /* value */
  union {
    /* constant value */
    Object *value;
    /* offset of variable */
    int offset;
    /* code object */
    Object *code;
    /* type object */
    TypeObject *type;
    /* enum value */
    int enumval;
  };
};

struct member *new_member(int kind, char *name, TypeDesc *type);
void free_member(struct member *m);

/* member table */
struct mtable {
  struct hashmap tbl;
  int counts[MBR_MAX];
};


/* function's proto, c and koala proto are the same. */
typedef Object *(*cfunc_t)(Object *, Object *);

void mtable_init(struct mtable *mtbl);
void mtable_fini(struct mtable *mtbl);
void mtable_add_const(struct mtable *mtbl, char *name,
                      TypeDesc *type, Object *val);
void mtable_add_var(struct mtable *mtbl, char *name, TypeDesc *type);
void mtable_add_func(struct mtable *mtbl, char *name, Object *code);

struct cfuncdef {
  char *name;
  char *ptype;
  char *rtype;
  cfunc_t func;
};

void mtable_add_cfuncs(struct mtable *mtbl, struct cfuncdef *funcs);

/* object header */
#define OBJECT_HEAD      \
  /* reference count */  \
  int ob_refcnt;         \
  /* type structure */   \
  TypeObject *ob_type;

/* root object */
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

#define OB_TYPE_ASSERT(_ob_, _type_) \
  assert(OB_TYPE(_ob_) == (_type_))

#define OB_INCREF(_ob_)          \
({                               \
  Object *ob = (Object *)(_ob_); \
  if (ob != NULL)                \
    ob->ob_refcnt++;             \
  (_ob_);                        \
})

#define OB_DECREF(_ob_)          \
({                               \
  Object *ob = (Object *)(_ob_); \
  if (ob != NULL) {              \
    --ob->ob_refcnt;             \
    assert(ob->ob_refcnt >= 0);  \
    if (ob->ob_refcnt <= 0) {    \
      OB_TYPE(ob)->freefn(ob);   \
      (_ob_) = NULL;             \
    }                            \
  }                              \
})

/* mark and sweep callback */
typedef void (*ob_mark_fn)(Object *);

/* alloc object function */
typedef Object *(*ob_alloc_fn)(TypeObject *);

/* free object function */
typedef void (*ob_free_fn)(Object *);

#define TYPE_OBJECT_CLASS 0
#define TYPE_OBJECT_TRAIT 1
#define TYPE_OBJECT_ENUM  2

/* typeobject structure */
struct typeobject {
  OBJECT_HEAD
  /* type name */
  char *name;
  /* class, trait or enum */
  int kind;

  /* mark-and-sweep */
  ob_mark_fn markfn;

  /* alloc and free */
  ob_alloc_fn allocfn;
  ob_free_fn freefn;

  /* direct super class and traits */
  struct vector base;
  /* line resolution order */
  struct vector lro;

  /* total fields, self & super */
  int total;
  /* number of self fields */
  int fields;
  /* member table */
  struct mtable mtbl;
};

extern TypeObject type_type;
extern TypeObject any_type;
extern Object nil_object;
void init_typeobject(void);
void fini_typeobject(void);

#define klass_add_const(ob, name, type, val)      \
({                                                \
  OB_TYPE_ASSERT(ob, &type_type);                \
  TypeObject *type = (TypeObject *)(ob);          \
  mtable_add_const(&type->mtbl, name, type, val); \
})

#define klass_add_val(ob, name, type)        \
({                                           \
  OB_TYPE_ASSERT(ob, &type_type);           \
  TypeObject *type = (TypeObject *)(ob);     \
  mtable_add_val(&type->mtbl, name, type);   \
})

#define klass_add_func(ob, name, code)       \
({                                           \
  OB_TYPE_ASSERT(ob, &type_type);           \
  TypeObject *type = (TypeObject *)(ob);     \
  mtable_add_func(&type->mtbl, name, code);  \
})

#define klass_add_cfuncs(ob, funcs)          \
({                                           \
  OB_TYPE_ASSERT(ob, &type_type);           \
  TypeObject *type = (TypeObject *)(ob);     \
  mtable_add_cfuncs(&type->mtbl, funcs);     \
})

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
