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
  /* its type */
  struct typedesc *type;
  /* value */
  union {
    /* constant value */
    struct object *value;
    /* offset of variable */
    int offset;
    /* code object */
    struct object *code;
    /* klass object */
    struct klass *klazz;
    /* enum value */
    int enumval;
  };
};

struct member *new_member(int kind, char *name, struct typedesc *type);
void free_member(struct member *m);

/* member table */
struct mtable {
  struct hashmap tbl;
  int counts[MBR_MAX];
};

struct klass;
struct object;

/* function's proto, c and koala proto are the same. */
typedef struct object *(*cfunc_t)(struct object *, struct object *);

void mtable_init(struct mtable *mtbl);
void mtable_free(struct mtable *mtbl);
void mtable_add_const(struct mtable *mtbl, char *name,
                      struct typedesc *type, struct object *val);
void mtable_add_var(struct mtable *mtbl, char *name, struct typedesc *type);
void mtable_add_func(struct mtable *mtbl, char *name, struct object *code);

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
  /* class structure */  \
  struct klass *ob_type;

/* root object */
struct object {
  OBJECT_HEAD
};

#define OBJECT_HEAD_INIT(_type_) \
  .ob_refcnt = 1, .ob_type = (_type_),

#define init_object_head(_ob_, _type_)          \
({                                              \
  struct object *ob = (struct object *)(_ob_);  \
  ob->ob_refcnt = 1;                            \
  ob->ob_type = (_type_);                       \
})

#define OB_TYPE(_ob_) \
  ((struct object *)(_ob_))->ob_type

#define OB_TYPE_ASSERT(_ob_, _type_) \
  assert(OB_TYPE(_ob_) == (_type_))

#define OB_INCREF(_ob)             \
({                                 \
  struct object *_o_ =             \
    (struct object *)(_ob);        \
  if (_o_ != NULL)                 \
    _o_->ob_refcnt++;              \
  (_ob);                           \
})

#define OB_DECREF(_ob)             \
({                                 \
  struct object *_o_ =             \
    (struct object *)(_ob);        \
  if (_o_ != NULL) {               \
    --_o_->ob_refcnt;              \
    assert(_o_->ob_refcnt >= 0);   \
    if (_o_->ob_refcnt <= 0) {     \
      OB_TYPE(_o_)->freefn(_o_);   \
      (_ob) = NULL;                \
    }                              \
  }                                \
})

/* mark and sweep callback */
typedef void (*ob_mark_fn)(struct object *);

/* alloc object function */
typedef struct object *(*ob_alloc_fn)(struct klass *);

/* free object function */
typedef void (*ob_free_fn)(struct object *);

#define KLASS_CLASS 0
#define KLASS_TRAIT 1
#define KLASS_ENUM  2

/* class structure */
struct klass {
  OBJECT_HEAD
  /* class name */
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

extern struct klass class_type;
extern struct klass any_type;
extern struct object nil_obj;
void init_klass_type(void);
void free_klass_type(void);

#define klass_add_const(ob, name, type, val)      \
({                                                \
  OB_TYPE_ASSERT(ob, &class_type);                \
  struct klass *type = (struct klass *)(ob);      \
  mtable_add_const(&type->mtbl, name, type, val); \
})

#define klass_add_val(ob, name, type)        \
({                                           \
  OB_TYPE_ASSERT(ob, &class_type);           \
  struct klass *type = (struct klass *)(ob); \
  mtable_add_val(&type->mtbl, name, type);   \
})

#define klass_add_func(ob, name, code)       \
({                                           \
  OB_TYPE_ASSERT(ob, &class_type);           \
  struct klass *type = (struct klass *)(ob); \
  mtable_add_func(&type->mtbl, name, code);  \
})

#define klass_add_cfuncs(ob, funcs)          \
({                                           \
  OB_TYPE_ASSERT(ob, &class_type);           \
  struct klass *type = (struct klass *)(ob); \
  mtable_add_cfuncs(&type->mtbl, funcs);     \
})

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_OBJECT_H_ */
