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

typedef struct klass Klass;
typedef struct object Object;

#define MBR_CONST 1
#define MBR_VAR   2
#define MBR_FUNC  3
#define MBR_PROTO 4
#define MBR_CLASS 5
#define MBR_TRAIT 6
#define MBR_ENUM  7
#define MBR_EVAL  8
#define MBR_MAX   9

/* member structure */
typedef struct mnode {
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
    /* offset of value */
    int offset;
    /* code object */
    Object *code;
    /* type object */
    Klass *type;
    /* enum value */
    int enumval;
  };
} MNode;

MNode *new_mnode(int kind, char *name, TypeDesc *type);
void free_mnode(MNode *m);

/* member table */
typedef struct mtable {
  struct hashmap tbl;
  int counts[MBR_MAX];
} MTable;

/* function's proto, c and koala proto are the same. */
typedef Object *(*cfunc_t)(Object *, Object *);

void mtbl_init(MTable *mtbl);
void mtbl_fini(MTable *mtbl);
void mtbl_add_const(MTable *mtbl, char *name, TypeDesc *type, Object *val);
void mtbl_add_var(MTable *mtbl, char *name, TypeDesc *type);
void mtbl_add_func(MTable *mtbl, char *name, Object *code);

struct cfuncdef {
  char *name;
  char *ptype;
  char *rtype;
  cfunc_t func;
};

void mtbl_add_cfuncs(MTable *mtbl, struct cfuncdef *funcs);

struct memberdef {
  char *name;
  char *type;
  cfunc_t func;
};

void mtbl_add_props(struct mtable *mtbl, struct memberdef *members);

/* object header */
#define OBJECT_HEAD      \
  /* reference count */  \
  int ob_refcnt;         \
  /* type structure */   \
  Klass *ob_type;

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
typedef void (*ob_markfunc)(Object *);

/* alloc object function */
typedef Object *(*ob_allocfunc)(Klass *);

/* free object function */
typedef void (*ob_freefunc)(Object *);

/* set function */
typedef void (*ob_setfunc)(Object *, char *name, Object *);

/* get function */
typedef Object *(*ob_getfunc)(Object *, char *name);

#define TYPE_OBJECT_CLASS 0
#define TYPE_OBJECT_TRAIT 1
#define TYPE_OBJECT_ENUM  2

/* klass structure */
struct klass {
  OBJECT_HEAD
  /* type name */
  char *name;
  /* class, trait or enum */
  int kind;

  /* mark-and-sweep */
  ob_markfunc markfn;

  /* alloc and free */
  ob_allocfunc allocfn;
  ob_freefunc freefn;

  /* setter & getter */
  ob_setfunc setfn;
  ob_getfunc getfn;

  /* direct super class and traits */
  struct vector base;
  /* line resolution order */
  struct vector lro;

  /* total attributes */
  int total;
  /* member table */
  MTable mtbl;
};

extern Klass class_type;
extern Klass any_type;
extern Object nil_object;
void init_typeobject(void);
void fini_typeobject(void);

#define klass_add_const(ob, name, type, val)      \
({                                                \
  OB_TYPE_ASSERT(ob, &class_type);                \
  Klass *type = (Klass *)(ob);                    \
  mtbl_add_const(&type->mtbl, name, type, val); \
})

#define klass_add_val(ob, name, type)        \
({                                           \
  OB_TYPE_ASSERT(ob, &class_type);           \
  Klass *type = (Klass *)(ob);               \
  mtbl_add_val(&type->mtbl, name, type);   \
})

#define klass_add_func(ob, name, code)       \
({                                           \
  OB_TYPE_ASSERT(ob, &class_type);           \
  Klass *type = (Klass *)(ob);               \
  mtbl_add_func(&type->mtbl, name, code);  \
})

#define klass_add_cfuncs(ob, funcs)          \
({                                           \
  OB_TYPE_ASSERT(ob, &class_type);           \
  Klass *type = (Klass *)(ob);               \
  mtbl_add_cfuncs(&type->mtbl, funcs);     \
})

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
