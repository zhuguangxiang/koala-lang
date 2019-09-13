/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef _KOALA_TYPEDESC_H_
#define _KOALA_TYPEDESC_H_

#include <inttypes.h>
#include "common.h"
#include "vector.h"
#include "strbuf.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BASE_BYTE  'b'
#define BASE_CHAR  'c'
#define BASE_INT   'i'
#define BASE_FLOAT 'f'
#define BASE_BOOL  'z'
#define BASE_STR   's'
#define BASE_ANY   'A'

/* constant value */
typedef struct literal {
  /* see BASE_XXX */
  char kind;
  union {
    wchar cval;
    int64_t ival;
    double fval;
    int bval;
    char *str;
  };
} Literal;

typedef struct typedesc TypeDesc;

typedef enum desckind {
  TYPE_INVALID,
  TYPE_BASE,
  TYPE_KLASS,
  TYPE_PROTO,
  TYPE_VARG,
  TYPE_PARAREF,
  TYPE_PARADEF,
  TYPE_ARRAY,
  TYPE_MAP,
  TYPE_TUPLE,
  TYPE_MAX,
} DescKind;

/*
 * Type descriptor
 * Klass: Lio.File;
 * Proto: Pis:e
 * Varg: ...s
 * Array: [s
 * Map: Mss
 */
struct typedesc {
  DescKind kind;
  int refcnt;
  Vector *paras;
  Vector *types;
  union {
     /* TYPE_BASE */
    char base;
    /* TYPE_KLASS */
    struct {
      char *path;
      char *type;
    } klass;
    /* TYPE_PROTO */
    struct {
      Vector *args;
      TypeDesc *ret;
    } proto;
    /* TYPE_PARAREF */
    struct {
      char *name;
      int index;
    } pararef;
    /* TYPE_PARADEF */
    struct {
      char *name;
      Vector *types;
    } paradef;
    /* TYPE_VARG  */
    TypeDesc *varg;
  };
};

#define TYPE_INCREF(desc) ({ \
  if (desc)                  \
    ++(desc)->refcnt;        \
  desc;                      \
})

#define TYPE_DECREF(desc) ({                    \
  if (desc) {                                   \
    --(desc)->refcnt;                           \
    bug((desc)->refcnt < 0, "type refcnt < 0"); \
    if ((desc)->refcnt == 0) {                  \
      desc_free(desc);                          \
      (desc) = NULL;                            \
    }                                           \
  }                                             \
})

void init_typedesc(void);
void fini_typedesc(void);
void literal_show(Literal *val, StrBuf *sbuf);
void desc_free(TypeDesc *desc);
void free_descs(Vector *vec);
void desc_tostr(TypeDesc *desc, StrBuf *buf);
char *base_str(int kind);
void desc_show(TypeDesc *desc);
int desc_check(TypeDesc *desc1, TypeDesc *desc2);

extern TypeDesc type_base_int;
extern TypeDesc type_base_str;
extern TypeDesc type_base_any;
extern TypeDesc type_base_bool;
extern TypeDesc type_base_byte;
extern TypeDesc type_base_char;
extern TypeDesc type_base_float;
extern TypeDesc type_base_desc;
#define desc_from_byte  TYPE_INCREF(&type_base_byte)
#define desc_from_int   TYPE_INCREF(&type_base_int)
#define desc_from_float TYPE_INCREF(&type_base_float)
#define desc_from_char  TYPE_INCREF(&type_base_char)
#define desc_from_str   TYPE_INCREF(&type_base_str)
#define desc_from_bool  TYPE_INCREF(&type_base_bool)
#define desc_from_any   TYPE_INCREF(&type_base_any)
#define desc_from_desc  TYPE_INCREF(&type_base_desc)
TypeDesc *desc_from_base(int kind);
TypeDesc *desc_from_klass(char *path, char *type);
TypeDesc *desc_from_proto(Vector *args, TypeDesc *ret);
TypeDesc *desc_from_array(TypeDesc *para);
TypeDesc *desc_from_varg(TypeDesc *base);
TypeDesc *desc_from_map(TypeDesc *key, TypeDesc *val);
TypeDesc *desc_from_tuple(Vector *types);
void klass_add_subtype(TypeDesc *desc, TypeDesc *subtype);
void desc_add_paradef(TypeDesc *desc, char *name, Vector *types);

TypeDesc *str_to_desc(char *s);
TypeDesc *str_to_proto(char *ptype, char *rtype);

#define desc_isint(desc)    ((desc) == &type_base_int)
#define desc_isbyte(desc)   ((desc) == &type_base_byte)
#define desc_isfloat(desc)  ((desc) == &type_base_float)
#define desc_ischar(desc)   ((desc) == &type_base_char)
#define desc_isstr(desc)    ((desc) == &type_base_str)
#define desc_isbool(desc)   ((desc) == &type_base_bool)
#define desc_isany(desc)    ((desc) == &type_base_any)
#define desc_istuple(desc)  ((desc)->kind == TYPE_TUPLE)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPEDESC_H_ */
