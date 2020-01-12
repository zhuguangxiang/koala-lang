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
  TYPE_PARAREF,
  TYPE_PARADEF,
  TYPE_LABEL,
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
  union {
     /* TYPE_BASE */
    char base;
    /* TYPE_KLASS */
    struct {
      char *type;
      char *path;
      Vector *typeargs;
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
      int index;
      Vector *typeparas;
    } paradef;
    /* TYPE_LABEL */
    struct {
      /* its enum type */
      TypeDesc *edesc;
      /* associated types */
      Vector *types;
    } label;
  };
};

#define TYPE_INCREF(desc) ({ \
  if (desc)                  \
    ++(desc)->refcnt;        \
  desc;                      \
})

#define TYPE_DECREF(desc) ({      \
  if (desc) {                     \
    --(desc)->refcnt;             \
    expect((desc)->refcnt >= 0);  \
    if ((desc)->refcnt == 0) {    \
      desc_free(desc);            \
      (desc) = NULL;              \
    }                             \
  }                               \
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
int check_subdesc(TypeDesc *desc1, TypeDesc *desc2);
int check_klassdesc(TypeDesc *desc1, TypeDesc *desc2);

extern TypeDesc type_base_int;
extern TypeDesc type_base_str;
extern TypeDesc type_base_any;
extern TypeDesc type_base_bool;
extern TypeDesc type_base_byte;
extern TypeDesc type_base_char;
extern TypeDesc type_base_float;
extern TypeDesc type_base_desc;
extern TypeDesc type_base_null;
#define desc_from_byte  TYPE_INCREF(&type_base_byte)
#define desc_from_int   TYPE_INCREF(&type_base_int)
#define desc_from_float TYPE_INCREF(&type_base_float)
#define desc_from_char  TYPE_INCREF(&type_base_char)
#define desc_from_str   TYPE_INCREF(&type_base_str)
#define desc_from_bool  TYPE_INCREF(&type_base_bool)
#define desc_from_any   TYPE_INCREF(&type_base_any)
#define desc_from_desc  TYPE_INCREF(&type_base_desc)
#define desc_from_null   TYPE_INCREF(&type_base_null)
TypeDesc *desc_from_base(int kind);
TypeDesc *desc_from_klass(char *path, char *type);
TypeDesc *desc_from_proto(Vector *args, TypeDesc *ret);
TypeDesc *desc_from_pararef(char *name, int index);
TypeDesc *desc_from_paradef(char *name, int index);
TypeDesc *desc_from_label(TypeDesc *edesc, Vector *types);
#define desc_from_varg  desc_from_klass("lang", "VArg")
#define desc_from_array desc_from_klass("lang", "Array")
#define desc_from_map   desc_from_klass("lang", "Map")
#define desc_from_tuple desc_from_klass("lang", "Tuple")
void desc_add_paratype(TypeDesc *desc, TypeDesc *type);
void desc_add_paradef(TypeDesc *desc, TypeDesc *type);
TypeDesc *desc_dup(TypeDesc *desc);

TypeDesc *str_to_desc(char *s);
TypeDesc *str_to_proto(char *ptype, char *rtype);

#define desc_isint(desc)    ((desc) == &type_base_int)
#define desc_isbyte(desc)   ((desc) == &type_base_byte)
#define desc_isfloat(desc)  ((desc) == &type_base_float)
#define desc_ischar(desc)   ((desc) == &type_base_char)
#define desc_isstr(desc)    ((desc) == &type_base_str)
#define desc_isbool(desc)   ((desc) == &type_base_bool)
#define desc_isany(desc)    ((desc) == &type_base_any)
#define desc_isbase(desc)   ((desc)->kind == TYPE_BASE)
#define desc_isproto(desc)  ((desc)->kind == TYPE_PROTO)
#define desc_istuple(desc) \
  ((desc)->kind == TYPE_KLASS && \
  (desc)->klass.path != NULL && \
  !strcmp((desc)->klass.path, "lang") && \
  !strcmp((desc)->klass.type, "Tuple"))
#define desc_isnull(desc)   ((desc) == &type_base_null)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPEDESC_H_ */
