/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _KOALA_OBJECT_H_
#define _KOALA_OBJECT_H_

#include "hashtable.h"
#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * forward declare Klass and Object
 */
typedef struct klass Klass;
typedef struct object Object;

/*
 * object header structure
 */
#define OBJECT_HEAD \
  int ob_refcnt; \
  Klass *ob_klass;

struct object {
  OBJECT_HEAD
};

/*
 * Macros for Object structure
 */
#define OBJECT_HEAD_INIT(klazz) \
  .ob_refcnt = 1, \
  .ob_klass = (klazz),
#define Init_Object_Head(ob, klazz) \
do { \
  Object *o = (Object *)(ob); \
  o->ob_refcnt = 1; \
  o->ob_klass = (klazz); \
} while (0)
#define OB_KLASS(ob)  (((Object *)(ob))->ob_klass)
#define OB_REFCNT(ob) (((Object *)(ob))->ob_refcnt)
#define OB_ASSERT_KLASS(ob, klazz) assert(OB_KLASS(ob) == &(klazz))

#define OB_INCREF(ob) (((Object *)(ob))->ob_refcnt++)
#define OB_DECREF(ob) \
do { \
  if (ob != NULL) { \
    if (--((Object *)(ob))->ob_refcnt != 0) { \
      assert(((Object *)(ob))->ob_refcnt > 0); \
    } else { \
      OB_KLASS(ob)->ob_free((Object *)ob); \
      (ob) = NULL; \
    } \
  } \
} while (0)

/*
 * function prototypes defined for Klass
 */
typedef void (*ob_markfunc)(Object *);
typedef Object *(*ob_allocfunc)(Klass *);
typedef void (*ob_freefunc)(Object *);
typedef uint32 (*ob_hashfunc)(Object *);
typedef int (*ob_equalfunc)(Object *, Object *);
typedef Object *(*ob_strfunc)(Object *);
typedef Object *(*ob_unaryfunc)(Object *);
typedef Object *(*ob_binaryfunc)(Object *, Object *);
typedef void (*ob_ternaryfunc)(Object *, Object *, Object *);
typedef Object *(*ob_getfunc)(Object *, Object *);
typedef void (*ob_setfunc)(Object *, Object *, Object *);

/*
 * basic operations, if necessary, these operators can be overridden .
 */
typedef struct {
  /* arithmetic operations */
  ob_binaryfunc add;
  ob_binaryfunc sub;
  ob_binaryfunc mul;
  ob_binaryfunc div;
  ob_binaryfunc mod;
  ob_unaryfunc  neg;
  /* relational operations */
  ob_binaryfunc gt;
  ob_binaryfunc ge;
  ob_binaryfunc lt;
  ob_binaryfunc le;
  ob_binaryfunc eq;
  ob_binaryfunc neq;
  /* bit operations */
  ob_binaryfunc band;
  ob_binaryfunc bor;
  ob_binaryfunc bxor;
  ob_unaryfunc  bnot;
  ob_binaryfunc lshift;
  ob_binaryfunc rshift;
  /* logic operations */
  ob_binaryfunc land;
  ob_binaryfunc lor;
  ob_unaryfunc  lnot;
} NumberOperations;

/*
 * map operations for array and map,
 * and other objects need '[]' operation, if they override '[]' operator.
 */
typedef struct {
  /* map's getter func, e.g. bar = map['foo']   */
  ob_binaryfunc get;
  /* map's setter func, e.g. map['foo'] = "bar" */
  ob_ternaryfunc set;
} MapOperations;

/* kind of member node */
typedef enum {
  CONST_KIND = 1,
  VAR_KIND   = 2,
  FUNC_KIND  = 3,
  PROTO_KIND = 4,
  CLASS_KIND = 5,
  TRAIT_KIND = 6,
} MemberKind;

/* Member node will be inserted into klass->table or package's table */
typedef struct membernode {
  HashNode hnode;
  /* Member's name, key */
  char *name;
  /* one of MemberKind */
  MemberKind kind;
  /* member's type descriptor */
  TypeDesc *desc;
  union {
    /* constant value */
    Object *value;
    /* offset of variable */
    int offset;
    /* code object */
    Object *code;
    /* class or trait */
    Klass *klazz;
  };
} MNode;

/* new Member node */
MNode *MNode_New(MemberKind kind, char *name, TypeDesc *desc);
/* free Member node */
void MNode_Free(MNode *node);
/* find member node from hashtable */
MNode *Find_MNode(HashTable *table, char *name);
/* member table tostring */
Object *MTable_ToString(HashTable *table);
/* initialize member table */
void Init_MTable(HashTable *table);
/* finalize member table */
void Fini_MTable(HashTable *table);

/* line resolution order node */
typedef struct lro_node {
  /* the class(trait) of this node */
  Klass *klazz;
  /* the fields' offset */
  int offset;
} LRONode;

#define OB_FLAGS_GC     1
#define OB_FLAGS_NUMOPS 2
#define OB_FLAGS_MAPOPS 4

/*
 * represent class or trait
 * a class is also an object, likely python
 */
struct klass {
  OBJECT_HEAD
  /* class name, for debug only */
  char *name;
  /* object's flags */
  int flags;
  /* total of vars, include base vars */
  int totalvars;

  /* for mark-and-sweep */
  ob_markfunc ob_mark;

  /* allocate and free functions */
  ob_allocfunc ob_alloc;
  ob_freefunc ob_free;

  /* used as key in map */
  ob_hashfunc ob_hash;
  ob_equalfunc ob_equal;

  /* like java's toString() */
  ob_strfunc ob_str;

  /* getter and setter */
  ob_getfunc ob_get;
  ob_setfunc Ob_set;

  /* number operations */
  NumberOperations *num_ops;
  /* map operations */
  MapOperations *map_ops;

  /* line resolution order(lro) */
  Vector lro;
  /* the class's owner */
  Object *owner;
  /* member table */
  HashTable mtbl;
  /* number of variables(fields) */
  int nrvars;
  /* constant pool */
  Object *consts;
};

/*
 * Skeleton of Klasses
 *                               Klass_Klass
 *                                   /|\
 *     |---------|---------|----------|----------|---------|-----------|
 * Nil_Klass Int_Klass Float_Klass Bool_Klass Any_Klass Trait_Klass Foo_Klass
 *                                                        /|\
 *                                                         |
 *                                                      Bar_Trait
 */
extern Klass Klass_Klass;
extern Klass Any_Klass;

/* new klass */
Klass *Klass_New(char *name, Vector *bases);
/* free klass */
void Klass_Free(Klass *klazz);
/* initialize klass */
void Init_Klass(Klass *klazz, Vector *bases);
/* finalize klass */
void Fini_Klass(Klass *klazz);
/* add a field to the klass(class or trait) */
int Klass_Add_Field(Klass *klazz, char *name, TypeDesc *desc);
/* add a method to the klass(class or trait) */
int Klass_Add_Method(Klass *klazz, Object *code);
/* add a prototype to trait only */
int Klass_Add_Proto(Klass *klazz, char *name, TypeDesc *proto);
/* klass to string */
Object *Klass_ToString(Object *ob);
/* get the field's value from the object */
Object *Get_Field(Object *ob, Klass *base, char *name);
/* set the field's value to the object */
void Set_Field(Object *ob, Klass *base, char *name, Object *val);
/* get a method from the object */
Object *Get_Method(Object *ob, Klass *base, char *name);

/*
 * function's proto, including c function and koala function
 * 'args' and 'return' are both Tuple.
 * Optimization: if it has only one parameter or one return value?
 */
typedef Object *(*cfunc_t)(Object *ob, Object *args);

/* c function definition, which can be called by koala function */
typedef struct cfunctiondef {
  /* func's name for searching the function */
  char *name;
  /* func's returns's type descriptor */
  char *rdesc;
  /* func's arguments's type descriptor */
  char *pdesc;
  /* func's code */
  cfunc_t func;
} CFunctionDef;

/*
 * add c functions to the class, which are defined as FuncDef structure,
 * with {NULL} as an end.
 */
int Klass_Add_CFunctions(Klass *klazz, CFunctionDef *functions);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OBJECT_H_ */
