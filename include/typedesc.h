/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
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
typedef struct constvalue {
  /* see BASE_XXX */
  char kind;
  union {
    wchar cval;
    int64_t ival;
    double fval;
    int bval;
    char *str;
  };
} ConstValue;

void constvalue_show(ConstValue *val, StrBuf *sbuf);

typedef enum desckind {
  TYPE_BASE = 1,
  TYPE_KLASS,
  TYPE_PROTO,
  TYPE_ARRAY,
  TYPE_MAP,
  TYPE_VARG,
  TYPE_TUPLE,
  TYPE_PARAREF,
} DescKind;

/*
 * Type descriptor
 * Klass: Lio.File;
 * Array: [s
 * Map: Mss
 * Proto: Pis:e;
 * Varg: ...s
 */
typedef struct typedesc {
  DescKind kind;
  int refcnt;
  union {
    struct {
      char type; /* one of BASE_XXX */
      char *str;
    } base;
    struct {
      char *path;
      char *type;
    } klass;
    struct {
      Vector *args;
      struct typedesc *ret;
    } proto;
  };
} TypeDesc;

#define TYPE_INCREF(desc) ({ \
  if (desc)                  \
    ++(desc)->refcnt;        \
  desc;                      \
})

#define TYPE_DECREF(desc) ({             \
  if (desc) {                            \
    --(desc)->refcnt;                    \
    if ((desc)->refcnt < 0)              \
      panic("type of '%d' refcnt error", \
            (desc)->kind);               \
    if ((desc)->refcnt <= 0) {           \
      desc_free(desc);                   \
      (desc) = NULL;                     \
    }                                    \
  }                                      \
})

void init_typedesc(void);
void fini_typedesc(void);
void desc_free(TypeDesc *desc);
void desc_tostring(TypeDesc *desc, StrBuf *buf);
int desc_equal(TypeDesc *desc1, TypeDesc *desc2);

TypeDesc *desc_getbase(int kind);

#define desc_from_byte()    desc_getbase(BASE_BYTE)
#define desc_from_integer() desc_getbase(BASE_INT)
#define desc_from_float()   desc_getbase(BASE_FLOAT)
#define desc_from_char()    desc_getbase(BASE_CHAR)
#define desc_from_string()  desc_getbase(BASE_STR)
#define desc_from_bool()    desc_getbase(BASE_BOOL)
#define desc_from_any()     desc_getbase(BASE_ANY)

TypeDesc *desc_getklass(char *path, char *type);
TypeDesc *desc_getproto(Vector *args, TypeDesc *ret);
TypeDesc *string_todesc(char *s);
TypeDesc *string_toproto(char *ptype, char *rtype);
Vector *string_todescs(char *s);
char *basedesc_str(int kind);
void desc_show(TypeDesc *desc);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPEDESC_H_ */
