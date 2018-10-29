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

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Type descriptor's kind
 * array and map are builtin types
 */
typedef enum typedesc_kind {
  TYPE_BASIC = 1, TYPE_KLASS, TYPE_PROTO, TYPE_ARRAY, TYPE_MAP
} TypeDescKind;

/*
 * basic types
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
#define BASIC_ANY    'a'
#define BASIC_VARG   'v'

/* Type descriptor */
typedef struct typedesc TypeDesc;

struct typedesc {
  TypeDescKind kind;
  union {
    /* one of BASIC_XXX */
    char basic;
    /* class or trait's absolute path */
    struct {
      String path;
      String type;
    } klass;
    /* function's proto */
    struct {
      Vector *arg;
      Vector *ret;
    } proto;
    /* map's key and val's type */
    struct {
      TypeDesc *key;
      TypeDesc *val;
    } map;
    /* array's dims and base type (subarray's type) */
    struct {
      int dims;
      TypeDesc *base;
    } array;
  };
};

/* basic type's defininitions */
extern TypeDesc Byte_Type;
extern TypeDesc Char_Type;
extern TypeDesc Int_Type;
extern TypeDesc Float_Type;
extern TypeDesc Bool_Type;
extern TypeDesc String_Type;
extern TypeDesc Any_Type;
extern TypeDesc Varg_Type;

/* check two typedescs are the same */
int TypeDesc_Equal(TypeDesc *t1, TypeDesc *t2);
/* convert typedesc struct to string for readable and printable */
void TypeDesc_ToString(TypeDesc *desc, char *buf);
/* copy the typedesc deeply */
TypeDesc *TypeDesc_Dup(TypeDesc *desc);
/* free the typedesc */
void TypeDesc_Free(TypeDesc *desc);
/* new a class or trait typedesc */
TypeDesc *TypeDesc_New_Klass(char *path, char *type);
/* new a proto typedesc */
TypeDesc *TypeDesc_New_Proto(Vector *arg, Vector *ret);
/* new a map typedesc */
TypeDesc *TypeDesc_New_Map(TypeDesc *key, TypeDesc *val);
/* new an array typedesc */
TypeDesc *TypeDesc_New_Array(int dims, TypeDesc *base);
/*
 * convert string to typedesc list
 * e.g. "i[sOkoala/lang.Tuple;" -->>> int, [string], koala/lang.Tuple
 */
Vector *String_To_TypeDescList(char *str);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TYPEDESC_H_ */
