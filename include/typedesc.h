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

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Type descriptor's kind
 * array and map are builtin types
 */
typedef enum desckind {
  TYPE_BASIC = 1,
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
#define BASIC_BYTE   'b'
#define BASIC_CHAR   'c'
#define BASIC_INT    'i'
#define BASIC_FLOAT  'f'
#define BASIC_BOOL   'z'
#define BASIC_STRING 's'
#define BASIC_ERROR  'e'
#define BASIC_ANY    'A'

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
  HashNode hnode; \
  DescKind kind; \
  String desc; \
  int refcnt;

typedef struct typedesc {
  TYPEDESC_HEAD
} TypeDesc;

#define TYPE_REFCNT(desc) (desc)->refcnt
#define TYPE_INCREF(desc) \
do { \
  if (desc != NULL) \
    ++(desc)->refcnt; \
} while (0)
#define TYPE_DECREF(desc) \
do { \
  --(desc)->refcnt; \
  assert(TYPE_REFCNT(desc) >= 0); \
} while (0)

typedef struct basicdesc {
  TYPEDESC_HEAD
  char type; /* one of BASIC_XXX */
} BasicDesc;

void Init_TypeDesc(void);
void Fini_TypeDesc(void);

/* class or trait */
typedef struct klassdesc {
  TYPEDESC_HEAD
  String path; /* absolute path not ref-name */
  String type;
} KlassDesc;

/* function's proto */
typedef struct protodesc {
  TYPEDESC_HEAD
  Vector *arg;
  Vector *ret;
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
void TypeDesc_ToString(TypeDesc *desc, char *buf);

/* free the typedesc */
void TypeDesc_Free(TypeDesc *desc);

/* get primitive typedesc */
TypeDesc *TypeDesc_Get_Basic(int basic);

/* new a class or trait typedesc */
TypeDesc *TypeDesc_Get_Klass(char *path, char *type);

/* new a proto typedesc */
TypeDesc *TypeDesc_Get_Proto(Vector *arg, Vector *ret);

/* new an array typedesc */
TypeDesc *TypeDesc_Get_Array(int dims, TypeDesc *base);

/* new a map typedesc */
TypeDesc *TypeDesc_Get_Map(TypeDesc *key, TypeDesc *val);

/* new a set typedesc */
TypeDesc *TypeDesc_Get_Set(TypeDesc *base);

/* new a var-argument typedesc */
TypeDesc *TypeDesc_Get_Varg(TypeDesc *base);

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
