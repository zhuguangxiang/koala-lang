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
 * some builtin klasses
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
extern Klass Trait_Klass;
extern Klass Int_Klass;
extern Klass Float_Klass;
extern Klass Bool_Klass;

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

/* line resolution order node */
typedef struct lro_node {
  /* the fields offset */
  int offset;
  /* the class(trait) of this node */
  Klass *klazz;
} LRONode;

/*
 * function prototypes defined for Klass
 */
typedef void (*markfunc)(Object *);
typedef Object *(*allocfunc)(Klass *);
typedef void (*freefunc)(Object *);
typedef uint32 (*hashfunc)(Object *);
typedef int (*equalfunc)(Object *, Object *);
typedef Object *(*strfunc)(Object *);
typedef Object *(*unaryfunc)(Object *);
typedef Object *(*binaryfunc)(Object *, Object *);
typedef void (*ternaryfunc)(Object *, Object *, Object *);

/*
 * basic operations, if necessary, these operators can be overridden .
 */
typedef struct {
  /* arithmetic operations */
  binaryfunc add;
  binaryfunc sub;
  binaryfunc mul;
  binaryfunc div;
  binaryfunc mod;
  unaryfunc  neg;
  /* relational operations */
  binaryfunc gt;
  binaryfunc ge;
  binaryfunc lt;
  binaryfunc le;
  binaryfunc eq;
  binaryfunc neq;
  /* bit operations */
  binaryfunc band;
  binaryfunc bor;
  binaryfunc bxor;
  unaryfunc  bnot;
  binaryfunc lshift;
  binaryfunc rshift;
  /* logic operations */
  binaryfunc land;
  binaryfunc lor;
  unaryfunc  lnot;
} NumberOperations;

/*
 * map operations for array and dictionary,
 * and other objects need '[]' operation, if they override '[]' operator.
 */
typedef struct {
  /* map's getter func, e.g. bar = dict['foo']   */
  binaryfunc get;
  /* map's setter func, e.g. dict['foo'] = "bar" */
  ternaryfunc set;
} MapOperations;

/* objects support garbage collection */
#define OB_FLAGS_GC  (1 << 1)

/*
 * represent class or trait
 * a class is also an object, likely python
 */
struct klass {
  OBJECT_HEAD
  /* class name, for debug only */
  char *name;
  /* object base size */
  int basesize;
  /* object item size, if it has variably itmes, e.g. array */
  int itemsize;
  /* self fields' offset */
  int offset;
  /* object's flags, one of OB_FLAGS_XXX */
  int flags;

  /* line resolution order(lro) for classes and traits inheritance */
  Vector lro;
  /* the class's owner */
  void *pkg;

  /* for gc */
  markfunc ob_mark;
  /* allocate and free functions */
  allocfunc ob_alloc;
  freefunc ob_free;
  /* used as key in dict */
  hashfunc ob_hash;
  equalfunc ob_equal;
  /* like java's toString() */
  strfunc ob_str;

  /* number operations, e.g. arithmetic operators */
  NumberOperations *numops;
  /* map operations for array and dictionary */
  MapOperations *mapops;

  /* fields and methods hash table */
  HashTable *table;
  /* consts pool ref to module's consts pool */
  Object *consts;
};

/* new common Klass */
Klass *Klass_New(char *name, Klass *base, Vector *traits, Klass *type);
/* free Klass */
void Klass_Free(Klass *klazz);
/* finalize Klass */
void Fini_Klass(Klass *klazz);
/* new class */
#define Class_New(name, base, traits) \
  Klass_New(name, base, traits, &Klass_Klass)
/* new trait */
#define Trait_New(name, traits) \
  Klass_New(name, NULL, traits, &Trait_Klass)
/* add a field to the klass(class or trait) */
int Klass_Add_Field(Klass *klazz, char *name, TypeDesc *desc);
/* add a method to the klass(class or trait) */
int Klass_Add_Method(Klass *klazz, char *name, Object *code);
/* add a prototype to trait only */
int Klass_Add_Proto(Klass *klazz, char *name, TypeDesc *proto);

/* get the field's value from the object, search base if possibly */
Object *Object_Get_Value(Object *ob, char *name, Klass *klazz);
/* set the field's value to the object, search base if possibly */
Object *Object_Set_Value(Object *ob, char *name, Klass *klazz, Object *val);
/*
 * get a method from the object, and return its real object
 * see 'struct object'
 */
Object *Object_Get_Method(Object *ob, char *name, Klass *klazz);

#define OB_INCREF(ob) (((Object *)(ob))->ob_refcnt++)
#define OB_DECREF(ob) \
do { \
  if (--((Object *)(ob))->ob_refcnt != 0) { \
    assert(((Object *)(ob))->ob_refcnt > 0); \
  } else { \
    OB_KLASS(ob)->ob_free(ob); \
    (ob) = NULL; \
  } \
} while (0)

/*
 * function's proto, including c function and koala function
 * 'args' and 'return' are both Tuple.
 * FIXME:
 * Maybe optimized it if it has only one parameter or one return value?
 */
typedef Object *(*cfunc)(Object *ob, Object *args);

/* c function definition, which can be called by koala function */
typedef struct funcdef {
  /* func's name for searching the function */
  char *name;
  /* func's returns's type descriptor */
  char *rdesc;
  /* func's arguments's type descriptor */
  char *pdesc;
  /* func's code */
  cfunc fn;
} FuncDef;

/* build codeobject from funcdef */
Object *FuncDef_Build_Code(FuncDef *f);

/*
 * add c functions to the class, which are defined as FuncDef structure,
 * with {NULL} as an end.
 */
int Klass_Add_CFunctions(Klass *klazz, FuncDef *funcs);

/* kind of MemberDef */
typedef enum {
  MEMBER_VAR   = 1,
  MEMBER_CODE  = 2,
  MEMBER_PROTO = 3,
  MEMBER_CLASS = 4,
  MEMBER_TRAIT = 5,
} MemberKind;

/* MemberDef will be inserted into klass->table or module's table */
typedef struct memberdef {
  /* hash node for hash table */
  HashNode hnode;
  /* one of MEMBER_XXX */
  MemberKind kind;
  /* Member's name, key */
  char *name;
  /* member's type descriptor */
  TypeDesc *desc;
  /* is constant? variable only */
  int k;
  /* by ->kind */
  union {
    /* offset of variable, field and prototype */
    int offset;
    /* func's code, koala's func or c's func */
    Object *code;
    /* class or trait */
    Klass *klazz;
  };
} MemberDef;

/* new common member, see MEMBER_XXX kind */
MemberDef *MemberDef_New(int kind, char *name, TypeDesc *desc, int k);
/* free MemberDef structure */
void MemberDef_Free(MemberDef *m);
/* new variable in module, class or trait */
MemberDef *MemberDef_Var_New(HashTable *table, char *name, TypeDesc *t, int k);
/* new code in package, class or trait */
MemberDef *MemberDef_Code_New(HashTable *table, char *name, Object *code);
/* new prototype in trait only */
#define MemberDef_Proto_New(name, desc) \
  MemberDef_New(MEMBER_PROTO, name, desc, 0)
/* new class in package */
#define MemberDef_Class_New(name) \
  MemberDef_New(MEMBER_CLASS, name, NULL, 0)
/* new trait in package */
#define MemberDef_Trait_New(name) \
  MemberDef_New(MEMBER_TRAIT, name, NULL, 0)
/* build a hashtable with MemberDef */
HashTable *MemberDef_Build_HashTable(void);
/* find memberdef from hashtable */
MemberDef *MemberDef_Find(HashTable *table, char *name);
Object *MemberDef_HashTable_ToString(HashTable *table);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_OBJECT_H_ */
