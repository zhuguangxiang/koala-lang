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
({                                  \
  Object *o = (Object *)(ob);       \
  o->ob_refcnt = 1;                 \
  o->ob_klass = (klazz);            \
})
#define OB_KLASS(ob)  (((Object *)(ob))->ob_klass)
#define OB_REFCNT(ob) (((Object *)(ob))->ob_refcnt)
#define OB_ASSERT_KLASS(ob, klazz) assert(OB_KLASS(ob) == &(klazz))

#define OB_INCREF(ob) (((Object *)(ob))->ob_refcnt++)
#define OB_DECREF(_ob)             \
({                                 \
  Object *obj = (Object *)(_ob);   \
  if (obj != NULL) {               \
    if (--obj->ob_refcnt != 0) {   \
      assert(obj->ob_refcnt > 0);  \
    } else {                       \
      OB_KLASS(obj)->ob_free(obj); \
      (_ob) = NULL;                \
    }                              \
  }                                \
})

/*
 * function prototypes(internal used only)
 */
typedef void (*ob_markfunc)(Object *);
typedef Object *(*ob_allocfunc)(Klass *);
typedef void (*ob_freefunc)(Object *);

/*
 * function's proto, including c function and koala function
 * 'args' and 'return' are both Tuple.
 * Optimization: if it has only one parameter or one return value?
 */
typedef Object *(*cfunc_t)(Object *ob, Object *args);

/*
 * basic operations, if necessary, these operators can be overridden .
 */
typedef struct {
  /* arithmetic operations */
  cfunc_t add;
  cfunc_t sub;
  cfunc_t mul;
  cfunc_t div;
  cfunc_t mod;
  cfunc_t pow;
  cfunc_t neg;
  /* relational operations */
  cfunc_t gt;
  cfunc_t ge;
  cfunc_t lt;
  cfunc_t le;
  cfunc_t eq;
  cfunc_t neq;
  /* bit operations */
  cfunc_t band;
  cfunc_t bor;
  cfunc_t bxor;
  cfunc_t bnot;
  cfunc_t lshift;
  cfunc_t rshift;
  /* logic operations */
  cfunc_t land;
  cfunc_t lor;
  cfunc_t lnot;
} NumberOperations;

/*
 * map operations for array and map,
 * and other objects need '[]' operation, if they override '[]' operator.
 */
typedef struct {
  /* map's getter func, e.g. bar = map['foo']   */
  cfunc_t get;
  /* map's setter func, e.g. map['foo'] = "bar" */
  cfunc_t set;
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
MNode *MNode_Find(HashTable *table, char *name);
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

  /* used as key */
  cfunc_t ob_hash;
  cfunc_t ob_cmp;

  /* like java's toString() */
  cfunc_t ob_str;

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

extern Klass Klass_Klass;
extern Klass Any_Klass;
void Init_Klass_Klass(void);
void Fini_Klass_Klass(void);
/* new klass */
Klass *Klass_New(char *name, Vector *bases);
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
/* get the field's value from the object */
Object *Object_Get_Field(Object *ob, Klass *base, char *name);
/* set the field's value to the object */
void Object_Set_Field(Object *ob, Klass *base, char *name, Object *val);
/* get a method from the object */
Object *Object_Get_Method(Object *ob, Klass *base, char *name);
/* object show */
Object *To_String(Object *ob);

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
