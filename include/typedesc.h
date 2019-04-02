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

#ifndef _KOALA_TYPEDESC_H_
#define _KOALA_TYPEDESC_H_

#include "vector.h"
#include "atomstring.h"
#include "hashtable.h"
#include "stringbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Type descriptor's kind
 * array and map are builtin types
 * NOTE: taged typedesc's head is only DescKind, not TYPEDESC_HEAD
 */
typedef enum desckind {
  TYPE_BASE = 1,
  TYPE_KLASS,
  TYPE_PROTO,
  TYPE_ARRAY,
  TYPE_MAP,
  TYPE_SET,
  TYPE_VARG
} DescKind;

/*
 * primitive types
 * integer: byte(1), int(8)
 * char: char(2 or 4?)
 * float: float(8)
 * any: builtin root object, like java's Object
 * varg: variably-argument, like c's ..., only in function's arguments
 */
#define BASE_BYTE   'b'
#define BASE_CHAR   'c'
#define BASE_INT    'i'
#define BASE_FLOAT  'f'
#define BASE_BOOL   'z'
#define BASE_STRING 's'
#define BASE_ANY    'A'
#define BASE_SELF   'S'

/* constant value */
typedef struct constvalue {
  /* see BASE_XXX in typedesc.h */
  int kind;
  union {
    uchar ch;
    int64 ival;
    float64 fval;
    int bval;
    char *str;
  };
} ConstValue;

void Const_Show(ConstValue *val, char *buf);

#define VARG_ANY_DESC "..."

/*
 * Type descriptor
 * Klass: Lio.File;
 * Array: [s
 * Map: Mss
 * Set: Si
 * Proto: Pis:Pi:i;ie;
 * Varg: ...s
 */
#define TYPEDESC_HEAD \
  DescKind kind;      \
  HashNode hnode;     \
  String desc;        \
  int refcnt;

typedef struct typedesc {
  TYPEDESC_HEAD
} TypeDesc;

#define TYPE_INCREF(desc) \
({                        \
  if (desc != NULL)       \
    ++(desc)->refcnt;     \
})

#define TYPE_DECREF(desc)             \
({                                    \
  TypeDesc *_desc = (TypeDesc *)desc; \
  if (_desc != NULL) {                \
    --(_desc)->refcnt;                \
    assert(_desc->refcnt >= 0);       \
    if (_desc->refcnt <= 0)           \
      TypeDesc_Free(_desc);           \
  }                                   \
})

void Init_TypeDesc(void);
void Fini_TypeDesc(void);

/* base type */
typedef struct basicdesc {
  TYPEDESC_HEAD
  int type;       /* one of BASE_XXX */
  char *pkgpath;  /* <path> */
  char *typestr;  /* <type> */
} BaseDesc;

/* class or trait */
typedef struct klassdesc {
  TYPEDESC_HEAD
  String path;   /* absolute path, but not ref-name */
  String type;
  Vector *paras; /* type parameters */
} KlassDesc;

/* function's proto */
typedef struct protodesc {
  TYPEDESC_HEAD
  Vector *arg;
  TypeDesc *ret;
} ProtoDesc;

/* array */
typedef struct arraydesc {
  TYPEDESC_HEAD
  int dims;
  TypeDesc *base;
} ArrayDesc;

/* map */
typedef struct mapdesc {
  TYPEDESC_HEAD
  TypeDesc *key;
  TypeDesc *val;
} MapDesc;

/* set */
typedef struct setdesc {
  TYPEDESC_HEAD
  TypeDesc *base;
} SetDesc;

/* varg type */
typedef struct vargdesc {
  TYPEDESC_HEAD
  TypeDesc *base;
} VargDesc;

/* check two typedescs are the same */
int TypeDesc_Equal(TypeDesc *desc1, TypeDesc *desc2);

/* convert typedesc struct to string for readable and printable */
String TypeDesc_ToString(TypeDesc *desc);

/* print ProtoDesc's arg and ret */
void Proto_Print(ProtoDesc *proto, StringBuf *buf);

/* free the typedesc */
void TypeDesc_Free(TypeDesc *desc);

/* get primitive typedesc */
TypeDesc *TypeDesc_Get_Base(int base);

/* new a class or trait typedesc */
TypeDesc *TypeDesc_Get_Klass(char *path, char *type, Vector *paras);

/* new a proto typedesc */
TypeDesc *TypeDesc_Get_Proto(Vector *arg, TypeDesc *ret);

/* new an array typedesc */
TypeDesc *TypeDesc_Get_Array(TypeDesc *base);

/* new a map typedesc */
TypeDesc *TypeDesc_Get_Map(TypeDesc *key, TypeDesc *val);

/* new a set typedesc */
TypeDesc *TypeDesc_Get_Set(TypeDesc *base);

/* new a var-argument typedesc */
TypeDesc *TypeDesc_Get_Varg(TypeDesc *base);

TypeDesc *__String_To_TypeDesc(char **string, int _dims, int _varg);
#define String_To_TypeDesc(s) \
({ \
  TypeDesc *desc = NULL; \
  if ((s) != NULL) { \
    char **str = &(s); \
    desc = __String_To_TypeDesc(str, 0, 0); \
  } \
  desc; \
})

/*
 * convert string to typedesc list
 * e.g. "i[sLlang.Tuple;" --->>> int, []string, lang.Tuple
 */
Vector *String_ToTypeList(char *str);

/*
 * convert typedesc list to string
 * e.g. int, []string, lang.Tuple --->>> "i[sLlang.Tuple;"
 */
String TypeList_ToString(Vector *vec);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TYPEDESC_H_ */
