/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

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

#include "hashmap.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

struct klass;

/* object header */
#define OBJECT_HEAD      \
  /* reference count */  \
  int ob_refcnt;         \
  /* class structure */  \
  struct klass *ob_klass;

/* root object */
struct object {
  OBJECT_HEAD
};

#define OBJECT_HEAD_INIT(_klass_) \
  .ob_refcnt = 1, .ob_klass = (_klass_),

#define OB_TYPE(_ob_) \
  ((struct object *)(_ob_))->ob_klass

#define OB_TYPE_ASSERT(_ob_, _klass_) \
  assert(OB_TYPE(_ob_) == (_klass_))

/* mark and sweep callback */
typedef void (*ob_mark_fn)(struct object *);

/* alloc object function */
typedef struct object *(*ob_alloc_fn)(struct klass *);

/* free object function */
typedef void (*ob_free_fn)(struct object *);

/* function's proto, c and koala proto are the same. */
typedef struct object *(*cfunc_fn)(struct object *, struct object *);

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
  struct hashmap mtbl;
};

#define MBR_CONST 1
#define MBR_VAR   2
#define MBR_FUNC  3
#define MBR_PROTO 4
#define MBR_CLASS 5
#define MBR_ENUM  6
#define MBR_EVAL  7

/* member structure of module or klass */
struct member {
  /* hashmap entry */
  struct hashmap_entry entry;
  /* member name */
  char *name;
  /* kind of member */
  int kind;
  /* its type */
  struct klass *type;
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

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OBJECT_H_ */
